//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "sgb.h"
#include "../savestate.h"

#include <cstring>
#include <algorithm>

namespace gambatte {

void saveStateCallback(unsigned char **o, void *i, std::size_t len) {
	std::memcpy(*o, i, len);
	*o += len;
}

void loadStateCallback(unsigned char **i, void *o, std::size_t len) {
	std::memcpy(o, *i, len);
	*i += len;
}

Sgb::Sgb()
: transfer(0xFF)
, pending(0xFF)
, spc(new SNES_SPC)
, samplesAccumulated_(0)
{
	// FIXME: this code is ugly
	unsigned i = 0;
	for (unsigned b = 0; b < 32; b++)
		for (unsigned g = 0; g < 32; g++)
			for (unsigned r = 0; r < 32; r++)
				cgbColorsRgb32_[i++] = ((b * 255 + 15) / 31) | (((g * 255 + 15) / 31) << 8) | (((r * 255 + 15) / 31) << 16) | 255 << 24;

	refreshPalettes();
	spc->init();
}

unsigned long Sgb::gbcToRgb32(unsigned const bgr15) {
	return cgbColorsRgb32_[bgr15 & 0x7FFF];
}

unsigned long Sgb::gbcToRgb32(unsigned const bgr15, unsigned const fade) {
	int const r = (bgr15 & 0x1F) - fade;
	int const g = ((bgr15 >> 5) & 0x1F) - fade;
	int const b = ((bgr15 >> 10) & 0x1F) - fade;
	return cgbColorsRgb32_[(std::max(r, 0) | (std::max(g, 0) << 5) | (std::max(b, 0) << 10)) & 0x7FFF];
}

void Sgb::setStatePtrs(SaveState &state) {
	state.mem.sgb.packet.set(packet, sizeof packet);
	state.mem.sgb.command.set(command, sizeof command);
	state.mem.sgb.frameBuf.set(frameBuf_, sizeof frameBuf_);
	state.mem.sgb.systemColors.set(systemColors, sizeof systemColors / sizeof systemColors[0]);
	state.mem.sgb.colors.set(colors, sizeof colors / sizeof colors[0]);
	state.mem.sgb.systemAttributes.set(systemAttributes, sizeof systemAttributes);
	state.mem.sgb.attributes.set(attributes, sizeof attributes);
	state.mem.sgb.systemTiles.set(systemTiles, sizeof systemTiles);
	state.mem.sgb.tiles.set(tiles, sizeof tiles);
	state.mem.sgb.systemTilemap.set(systemTilemap, sizeof systemTilemap / sizeof systemTilemap[0]);
	state.mem.sgb.tilemap.set(tilemap, sizeof tilemap / sizeof tilemap[0]);
	state.mem.sgb.systemTileColors.set(systemTileColors, sizeof systemTileColors / sizeof systemTileColors[0]);
	state.mem.sgb.tileColors.set(tileColors, sizeof tileColors / sizeof tileColors[0]);
	state.mem.sgb.spcState.set(spcState, sizeof spcState);
	state.mem.sgb.soundControl.set(soundControl, sizeof soundControl);

	saveSpcState();
}

void Sgb::saveState(SaveState &state) const {
	state.mem.sgb.transfer = transfer;
	state.mem.sgb.commandIndex = commandIndex;
	state.mem.sgb.joypadIndex = joypadIndex;
	state.mem.sgb.joypadMask = joypadMask;
	state.mem.sgb.borderFade = borderFade;
	state.mem.sgb.pending = pending;
	state.mem.sgb.pendingCount = pendingCount;
	state.mem.sgb.mask = mask;
	state.mem.sgb.samplesAccumulated = samplesAccumulated_;
}

void Sgb::loadState(SaveState const &state) {
	transfer = state.mem.sgb.transfer;
	commandIndex = state.mem.sgb.commandIndex;
	joypadIndex = state.mem.sgb.joypadIndex;
	joypadMask = state.mem.sgb.joypadMask;
	borderFade = state.mem.sgb.borderFade;
	pending = state.mem.sgb.pending;
	pendingCount = state.mem.sgb.pendingCount;
	mask = state.mem.sgb.mask;
	samplesAccumulated_ = state.mem.sgb.samplesAccumulated;

	refreshPalettes();
	loadSpcState();
}

void Sgb::saveSpcState() {
	unsigned char *o = spcState;
	spc->copy_state(&o, saveStateCallback);
}

void Sgb::loadSpcState() {
	spc->set_output(NULL, 0);
	unsigned char *i = spcState;
	spc->copy_state(&i, loadStateCallback);
}

void Sgb::onJoypad(unsigned data, unsigned write) {
	handleTransfer(data);

	if ((data & 0x20) == 0 && (write & 0x20) != 0)
		joypadIndex = (joypadIndex + 1) & joypadMask;
}

void Sgb::updateScreen() {
	unsigned char frame[160 * 144];
	
	for (unsigned j = 0; j < 144; j++) {
		for (unsigned i = 0; i < 160; i++)
			frame[j * 160 + i] = 3 - (videoBuf_[j * pitch_ + i] >> 4 & 3);
	}

	if (pending != 0xFF && --pendingCount == 0)
		onTransfer(frame);

	if (!mask)
		std::memcpy(frameBuf_, frame, sizeof frame);

	// border affects the colors within the gb area for whatever reason, so we need to go through that data too
	uint_least32_t frame_[256 * 224];
	uint_least32_t *gbFrame = &frame_[40 * 256 + 48];

	switch (mask) {
	case 0:
	case 1:
		for (unsigned j = 0; j < 144; j++) {
			for (unsigned i = 0; i < 160; i++) {
				unsigned attribute = attributes[(j / 8) * 20 + (i / 8)] & 3;
				gbFrame[j * 256 + i] = palette[attribute * 4 + frameBuf_[j * 160 + i]];
			}
		}
		break;
	case 2:
	{
		unsigned long const black = gbcToRgb32(0);
		for (unsigned j = 0; j < 144; j++) {
			for (unsigned i = 0; i < 160; i++)
				gbFrame[j * 256 + i] = black;
		}
		break;
	}
	case 3:
	{
		unsigned long const pal0 = palette[0];
		for (unsigned j = 0; j < 144; j++) {
			for (unsigned i = 0; i < 160; i++)
				gbFrame[j * 256 + i] = pal0;
		}
		break;
	}
	}

	if (borderFade && --borderFade == 32) {
		std::memcpy(systemTiles,      tiles,      sizeof tiles);
		std::memcpy(systemTilemap,    tilemap,    sizeof tilemap);
		std::memcpy(systemTileColors, tileColors, sizeof tileColors);
	}

	unsigned long colors[16 * 4];
	if (borderFade == 0) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i]);
	} else if (borderFade > 32) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], 64 - borderFade);
	} else {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], borderFade);
	}

	for (unsigned tileY = 0; tileY < 28; tileY++) {
		for (unsigned tileX = 0; tileX < 32; tileX++) {
			if (!(tileX >= 6 && tileX < 26 && tileY >= 5 && tileY < 23)) // border area
				continue;

			unsigned short tile = systemTilemap[tileX + tileY * 32];
			unsigned char flipX = (tile & 0x4000) ? 0 : 7;
			unsigned char flipY = (tile & 0x8000) ? 7 : 0;
			unsigned char pal = (tile >> 10) & 3;
			for (unsigned y = 0; y < 8; y++) {
				unsigned base = (tile & 0xFF) * 32 + (y ^ flipY) * 2;
				for (unsigned x = 0; x < 8; x++) {
					unsigned char bit = 1 << (x ^ flipX);
					unsigned char color =
					((systemTiles[base + 00] & bit) ? 1 : 0) |
					((systemTiles[base + 01] & bit) ? 2 : 0) |
					((systemTiles[base + 16] & bit) ? 4 : 0) |
					((systemTiles[base + 17] & bit) ? 8 : 0);

					uint_least32_t *pixel = &frame_[tileX * 8 + x + (tileY * 8 + y) * 256];
					if (color)
						*pixel = colors[color + pal * 16];
				}
			}
		}
	}

	for (unsigned j = 0; j < 144; j++) {
		for (unsigned i = 0; i < 160; i++)
			videoBuf_[j * pitch_ + i] = gbFrame[j * 256 + i];
	}
}

unsigned Sgb::updateScreenBorder(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
	if (!videoBuf_ || !videoBuf)
		return -1;

	uint_least32_t frame[256 * 224];

	unsigned long colors[16 * 4];
	if (borderFade == 0) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i]);
	} else if (borderFade > 32) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], 64 - borderFade);
	} else {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], borderFade);
	}

	uint_least32_t *gbFrame = &frame[40 * 256 + 48];
	for (unsigned j = 0; j < 144; j++) {
		for (unsigned i = 0; i < 160; i++)
			gbFrame[j * 256 + i] = videoBuf_[j * pitch_ + i];
	}

	for (unsigned tileY = 0; tileY < 28; tileY++) {
		for (unsigned tileX = 0; tileX < 32; tileX++) {
			if (tileX >= 6 && tileX < 26 && tileY >= 5 && tileY < 23) // gb area
				continue;

			unsigned short tile = systemTilemap[tileX + tileY * 32];
			unsigned char flipX = (tile & 0x4000) ? 0 : 7;
			unsigned char flipY = (tile & 0x8000) ? 7 : 0;
			unsigned char pal = (tile >> 10) & 3;
			for (unsigned y = 0; y < 8; y++) {
				unsigned base = (tile & 0xFF) * 32 + (y ^ flipY) * 2;
				for (unsigned x = 0; x < 8; x++) {
					unsigned char bit = 1 << (x ^ flipX);
					unsigned char color =
					((systemTiles[base + 00] & bit) ? 1 : 0) |
					((systemTiles[base + 01] & bit) ? 2 : 0) |
					((systemTiles[base + 16] & bit) ? 4 : 0) |
					((systemTiles[base + 17] & bit) ? 8 : 0);

					uint_least32_t *pixel = &frame[tileX * 8 + x + (tileY * 8 + y) * 256];
					if (color == 0)
						*pixel = palette[0];
					else
						*pixel = colors[color + pal * 16];
				}
			}
		}
	}

	for (unsigned j = 0; j < 224; j++) {
		for (unsigned i = 0; i < 256; i++)
			videoBuf[j * pitch + i] = frame[j * 256 + i];
	}

	return 0;
}

void Sgb::handleTransfer(unsigned data) {
	if ((data & 0x30) == 0) {
		std::memset(packet, 0, sizeof packet);
		transfer = 0;
	} else if (transfer != 0xFF) {
		if (transfer < 128) {
			packet[transfer >> 3] |= ((data & 0x20) == 0) << (transfer & 7);
			transfer++;
		} else if ((data & 0x10) == 0) {
			transfer = 0xFF;
			std::memcpy(command + commandIndex * sizeof packet, packet, sizeof packet);

			if (++commandIndex == (command[0] & 7)) {
				onCommand();
				commandIndex = 0;
			}
		}
	}
}

void Sgb::onCommand() {
	if ((command[0] & 7) == 0)
		return;

	switch (command[0] >> 3) {
	case PAL01:
		palnn(0, 1);
		break;
	case PAL23:
		palnn(2, 3);
		break;
	case PAL03:
		palnn(0, 3);
		break;
	case PAL12:
		palnn(1, 2);
		break;
	case ATTR_BLK:
		attrBlk();
		break;
	case ATTR_LIN:
		attrLin();
		break;
	case ATTR_DIV:
		attrDiv();
		break;
	case ATTR_CHR:
		attrChr();
		break;
	case PAL_SET:
		palSet();
		break;
	case SOUND:
		cmdSound();
		break;
	case SOU_TRN:
		pending = SOU_TRN;
		pendingCount = 2;
		break;
	case PAL_TRN:
		pending = PAL_TRN;
		pendingCount = 2;
		break;
	case CHR_TRN:
		pending = CHR_TRN;
		if (command[1] & 1)
			pending |= HIGH;

		pendingCount = 2;
		break;
	case PCT_TRN:
		pending = PCT_TRN;
		pendingCount = 2;
		break;
	case ATTR_TRN:
		pending = ATTR_TRN;
		pendingCount = 2;
		break;
	case MLT_REQ:
		joypadMask = (command[1] & 2) | !!(command[1] & 3);
		joypadIndex &= joypadMask;
		break;
	case ATTR_SET:
		attrSet();
		break;
	case MASK_EN:
		mask = command[1] & 3;
		break;
	}
}

void Sgb::onTransfer(unsigned char *frame) {
	unsigned char vram[4096];

	for (unsigned i = 0; i < 256; i++) {
		unsigned const y = (i / 20);
		unsigned const x = (i % 20);
		unsigned char *src = frame + y * 8 * 160 + x * 8;
		unsigned char *dst = vram + 16 * i;

		for (unsigned j = 0; j < 8; j++) {
			unsigned char a = 0;
			unsigned char b = 0;

			a |= (src[7] & 1) << 0;
			a |= (src[6] & 1) << 1;
			a |= (src[5] & 1) << 2;
			a |= (src[4] & 1) << 3;
			a |= (src[3] & 1) << 4;
			a |= (src[2] & 1) << 5;
			a |= (src[1] & 1) << 6;
			a |= (src[0] & 1) << 7;

			b |= (src[7] & 2) >> 1;
			b |= (src[6] & 2) << 0;
			b |= (src[5] & 2) << 1;
			b |= (src[4] & 2) << 2;
			b |= (src[3] & 2) << 3;
			b |= (src[2] & 2) << 4;
			b |= (src[1] & 2) << 5;
			b |= (src[0] & 2) << 6;

			*dst++ = a;
			*dst++ = b;
			src += 160;
		}
	}

	switch (pending) {
	case SOU_TRN:
	{
		unsigned char *end = &vram[sizeof vram];
		unsigned char *src = vram;
		unsigned char *dst = spc->get_ram();
		
		while (true) {
			if (src + 4 >= end)
				break;

			unsigned len = src[0] | src[1] << 8;
			unsigned addr = src[2] | src[3] << 8;
			if (!len)
				break;

			src += 4;
			if ((src + len) >= end || (addr + len) >= 0x10000)
				break;

			std::memcpy(dst + addr, src, len);
			src += len;
		}
		break;
	}
	case PAL_TRN:
	{
		for (unsigned i = 0; i < 2048; i++)
			systemColors[i] = vram[i * 2] | vram[i * 2 + 1] << 8;

		break;
	}
	case CHR_TRN | HIGH:
	case CHR_TRN:
	{
		unsigned char *tileAddr = (pending & HIGH) ? &tiles[4096] : &tiles[0];
		unsigned short *src = (unsigned short *)vram;
		unsigned short *dst = (unsigned short *)tileAddr;
		for (unsigned i = 0; i < 2048; i++)
			dst[i] = src[i];

		break;
	}
	case PCT_TRN:
	{
		unsigned short *src = (unsigned short *)vram;
		unsigned short *dst = tilemap;
		for (unsigned i = 0; i < 32 * 32; i++)
			dst[i] = src[i];

		src += sizeof tilemap / sizeof tilemap[0];
		dst = tileColors;
		for (unsigned i = 0; i < 16 * 4; i++)
			dst[i] = src[i];

		borderFade = 64;
		break;
	}
	case ATTR_TRN:
	{
		unsigned short *src = (unsigned short *)vram;
		unsigned short *dst = (unsigned short *)systemAttributes;
		for (unsigned i = 0; i < (45 * 90) / 2; i++)
			dst[i] = src[i];

		break;
	}
	}

	pending = 0xFF;
}

void Sgb::refreshPalettes() {
	for (unsigned i = 0; i < 16; i++)
		palette[i] = gbcToRgb32(colors[i * ((i & 3) != 0)]);
}

void Sgb::palnn(unsigned a, unsigned b) {
	unsigned short color[7];
	for (unsigned i = 0; i < 7; i++)
		color[i] = command[1 + i * 2] | command[1 + i * 2 + 1] << 8;

	colors[0 * 4 + 0] = color[0];
	colors[1 * 4 + 0] = color[0];
	colors[2 * 4 + 0] = color[0];
	colors[3 * 4 + 0] = color[0];
	colors[a * 4 + 1] = color[1];
	colors[a * 4 + 2] = color[2];
	colors[a * 4 + 3] = color[3];
	colors[b * 4 + 1] = color[4];
	colors[b * 4 + 2] = color[5];
	colors[b * 4 + 3] = color[6];

	refreshPalettes();
}

void Sgb::attrBlk() {
	unsigned nset = command[1];
	if (nset >= 19)
		return;
	
	for (unsigned i = 0; i < nset; i++) {
		unsigned ctrl = command[2 + i * 6 + 0] & 7;
		unsigned pals = command[2 + i * 6 + 1];
		unsigned x1 = command[2 + i * 6 + 2] & 31;
		unsigned y1 = command[2 + i * 6 + 3] & 31;
		unsigned x2 = command[2 + i * 6 + 4] & 31;
		unsigned y2 = command[2 + i * 6 + 5] & 31;

		unsigned inside = ctrl & 1;
		unsigned line = ctrl & 2;
		unsigned outside = ctrl & 4;

		unsigned char insidepal = pals & 3;
		unsigned char linepal = pals >> 2 & 3;
		unsigned char outsidepal = pals >> 4 & 3;

		if (ctrl == 1) {
			line = 2;
			linepal = insidepal;
		} else if (ctrl == 4) {
			line = 2;
			linepal = outsidepal;
		}

		for (unsigned j = 0; j < 20 * 18; j++) {
			unsigned x = j % 20;
			unsigned y = j / 20;

			if (x < x1 || y < y1 || x > x2 || y > y2) {
				if (outside)
					attributes[j] = outsidepal;
			} else if (x == x1 || y == y1 || x == x2 || y == y2) {
				if (line)
					attributes[j] = linepal;
			} else if (inside)
				attributes[j] = insidepal;
		}
	}
}

void Sgb::attrLin() {
	unsigned nset = command[1];
	if (nset >= 111)
		return;

	for (unsigned i = 0; i < nset; i++) {
		unsigned char v = command[i + 2];
		unsigned line = v & 31;
		unsigned pal = v >> 5 & 3;
		if (v & 0x80) { // horizontal
			if (line > 18)
				continue;

			std::memset(attributes + line * 20, pal, 20);
		} else { // vertical
			if (line > 20)
				continue;

			unsigned char *dst = attributes + line;
			for (unsigned i = 0; i < 18; i++, dst += 20)
				dst[0] = pal;
		}
	}
}

void Sgb::attrDiv() {
	unsigned char v = command[1];

	unsigned char highpal = v & 3;
	unsigned char lowpal = v >> 2 & 3;
	unsigned char linepal = v >> 4 & 3;

	unsigned pos = command[2] & 31;
	if (v & 0x40) { // horizontal
		for (unsigned i = 0; i < 18 * 20; i++) {
			unsigned y = i / 20;
			if (y < pos)
				attributes[i] = lowpal;
			else if (y == pos)
				attributes[i] = linepal;
			else
				attributes[i] = highpal;
		}
	} else { // vertical
		for (unsigned i = 0; i < 18 * 20; i++) {
			unsigned x = i % 20;
			if (x < pos)
				attributes[i] = lowpal;
			else if (x == pos)
				attributes[i] = linepal;
			else
				attributes[i] = highpal;
		}
	}
}

void Sgb::attrChr() {
	unsigned x = command[1];
	unsigned y = command[2];
	unsigned n = command[3] | command[4] << 8;

	if (x > 19 || y > 17)
		return;

	unsigned vertical = command[5];
	for (unsigned i = 0; i < n; i++) {
		unsigned char v = command[i / 4 + 6];
		unsigned char pal = (v >> (((~i) & 3) << 1)) & 3;
		attributes[y * 20 + x] = pal;
		if (vertical) {
			y++;
			if (y == 18) {
				x++;
				y = 0;
				if (x == 20)
					break;
			}
		} else {
			x++;
			if (x == 20) {
				y++;
				x = 0;
				if (y == 18)
					break;
			}
		}
	}
}

void Sgb::attrSet() {
	unsigned attr = command[1] & 0x3F;
	if (attr >= 45)
		return;

	for (unsigned i = 0; i < 90; i++) {
		unsigned char b = systemAttributes[attr * 90 + i];
		for (unsigned j = 0; j < 4; j++) {
			attributes[j + i * 4] = b >> 6;
			b <<= 2;
		}
	}
	if (command[1] & 0x40)
		mask = 0;
}

void Sgb::palSet() {
	unsigned p0 = command[1] | ((command[2] << 8) & 0x100);
	for (unsigned i = 0; i < 4; i++) {
		unsigned p = command[1 + i * 2] | ((command[1 + i * 2 + 1] << 8) & 0x100);
		colors[i * 4 + 0] = systemColors[p0 * 4 + 0];
		colors[i * 4 + 1] = systemColors[p  * 4 + 1];
		colors[i * 4 + 2] = systemColors[p  * 4 + 2];
		colors[i * 4 + 3] = systemColors[p  * 4 + 3];
	}
	if (command[9] & 0x80) {
		unsigned attr = command[9] & 0x3F;
		if (attr >= 45)
			attr = 44;
		
		for (unsigned i = 0; i < 90; i++) {
			unsigned char b = systemAttributes[attr * 90 + i];
			for (unsigned j = 0; j < 4; j++) {
				attributes[j + i * 4] = b >> 6;
				b <<= 2;
			}
		}
	}
	if (command[9] & 0x40)
		mask = 0;

	refreshPalettes();
}

void Sgb::cmdSound() {
	soundControl[1] = command[1];
	soundControl[2] = command[2];
	soundControl[3] = command[3];
	soundControl[0] = command[4];
}

unsigned Sgb::generateSamples(short *soundBuf, std::size_t &samples) {
	samples = samplesAccumulated_ / 65;
	samplesAccumulated_ -= samples * 65;
	short buf[2048 * 2];
	spc->set_output(buf, 2048);
	bool matched = true;
	for (unsigned p = 0; p < 4; p++) {
		if (spc->read_port(0, p) != soundControl[p])
			matched = false;
	}
	if (matched) {
		soundControl[0] = 0;
		soundControl[1] = 0;
		soundControl[2] = 0;
	}
	for (unsigned p = 0; p < 4; p++)
		spc->write_port(0, p, soundControl[p]);

	spc->end_frame(samples * 32);
	if (soundBuf)
		std::memcpy(soundBuf, buf, sizeof buf);

	return samplesAccumulated_;
}

SYNCFUNC(Sgb) {
	NSS(transfer);
	NSS(packet);
	NSS(command);
	NSS(commandIndex);

	NSS(joypadIndex);
	NSS(joypadMask);

	NSS(frameBuf_);

	NSS(systemColors);
	NSS(colors);
	NSS(systemAttributes);
	NSS(attributes);

	NSS(systemTiles);
	NSS(tiles);
	NSS(systemTilemap);
	NSS(tilemap);
	NSS(systemTileColors);
	NSS(tileColors);
	NSS(borderFade);

	NSS(pending);
	NSS(pendingCount);
	NSS(mask);

	if (!isReader)
		saveSpcState();

	NSS(spcState);

	if (isReader)
		loadSpcState();

	NSS(soundControl);
	NSS(samplesAccumulated_);
}

}
