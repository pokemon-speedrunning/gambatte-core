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
#include "ipl.h"
#include "spc_data.h"

#include <cstring>
#include <stdio.h>

namespace gambatte {

Sgb::Sgb()
: transfer(0xFF)
, pending(0xFF)
, spc(spc_new())
, buffer_(0)
, lastUpdate_(0)
{
	// FIXME: this code is ugly
	int i = 0;
	for (int b = 0; b < 32; b++)
		for (int g = 0; g < 32; g++)
			for (int r = 0; r < 32; r++)
				cgbColorsRgb32_[i++] = ((b * 255 + 15) / 31) | (((g * 255 + 15) / 31) << 8) | (((r * 255 + 15) / 31) << 16) | 255 << 24;

	spc_init_rom(spc, iplRom);
	spc_reset(spc);
	spc_load_spc(spc, spcData, 0x10224);

	short buf[4096];
	spc_set_output(spc, buf, 4096);
	for (unsigned i = 0; i < 240; i++)
		spc_end_frame(spc, 35104);
}

unsigned long Sgb::gbcToRgb32(unsigned const bgr15) {
	return cgbColorsRgb32_[bgr15 & 0x7FFF];
}

void Sgb::setStatePtrs(SaveState &state) {
	state.mem.sgb.packet.set(packet, sizeof packet);
	state.mem.sgb.command.set(command, sizeof command);
	state.mem.sgb.systemColors.set(systemColors, sizeof systemColors / sizeof systemColors[0]);
	state.mem.sgb.colors.set(colors, sizeof colors / sizeof colors[0]);
	state.mem.sgb.attributes.set(attributes, sizeof attributes);
}

void Sgb::saveState(SaveState &state) const {
	state.mem.sgb.transfer = transfer;
	state.mem.sgb.commandIndex = commandIndex;
	state.mem.sgb.joypadIndex = joypadIndex;
	state.mem.sgb.joypadMask = joypadMask;
	state.mem.sgb.pending = pending;
	state.mem.sgb.pendingCount = pendingCount;
	state.mem.sgb.mask = mask;
}

void Sgb::loadState(SaveState const &state) {
	transfer = state.mem.sgb.transfer;
	commandIndex = state.mem.sgb.commandIndex;
	joypadIndex = state.mem.sgb.joypadIndex;
	joypadMask = state.mem.sgb.joypadMask;
	pending = state.mem.sgb.pending;
	pendingCount = state.mem.sgb.pendingCount;
	mask = state.mem.sgb.mask;

	refreshPalettes();
}

void Sgb::onJoypad(unsigned data) {
	handleTransfer(data);

	if ((data & 0x20) == 0)
		joypadIndex = (joypadIndex + 1) & joypadMask;
}

void Sgb::updateScreen() {
	unsigned char frame[160 * 144];
	
	for (int j = 0; j < 144; j++) {
		for (int i = 0; i < 160; i++)
			frame[j * 160 + i] = 3 - (videoBuf_[j * pitch_ + i] >> 4 & 3);
	}

	if (pending != 0xFF && --pendingCount == 0)
		onTransfer(frame);

	if (mask != 0)
		memset(frame, 0, sizeof frame);

	for (int j = 0; j < 144; j++) {
		for (int i = 0; i < 160; i++) {
			unsigned attribute = attributes[(j / 8) * 20 + (i / 8)];
			videoBuf_[j * pitch_ + i] = palette[attribute * 4 + frame[j * 160 + i]];
		}
	}
}

void Sgb::handleTransfer(unsigned data) {
	if ((data & 0x30) == 0) {
		memset(packet, 0, sizeof packet);
		transfer = 0;
	} else if (transfer != 0xFF) {
		if (transfer < 128) {
			packet[transfer >> 3] |= ((data & 0x20) == 0) << (transfer & 7);
			transfer++;
		} else if ((data & 0x10) == 0) {
			transfer = 0xFF;
			memcpy(command + commandIndex * sizeof packet, packet, sizeof packet);

			if (++commandIndex == (command[0] & 7)) {
				onCommand();
				commandIndex = 0;
			}
		}
	}
}

void Sgb::onCommand() {
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
		if ((command[0] & 7) == 1 && pending == 0xFF) {
			pending = SOU_TRN;
			pendingCount = 4;
		}
		break;
	case PAL_TRN:
		if ((command[0] & 7) == 1 && pending == 0xFF) {
			pending = PAL_TRN;
			pendingCount = 4;
		}
		break;
	case CHR_TRN:
		if ((command[0] & 7) == 1 && pending == 0xFF) {
			pending = CHR_TRN;
			if (command[1] & 1)
				pending |= HIGH;

			pendingCount = 4;
		}
		break;
	case PCT_TRN:
		if ((command[0] & 7) == 1 && pending == 0xFF) {
			pending = PCT_TRN;
			pendingCount = 4;
		}
		break;
	case ATTR_TRN:
		if ((command[0] & 7) == 1 && pending == 0xFF) {
			pending = ATTR_TRN;
			pendingCount = 4;
		}
		break;
	case MLT_REQ:
		joypadMask = command[1];
		joypadIndex &= joypadMask;
		break;
	case ATTR_SET:
		attrSet();
		break;
	case MASK_EN:
		mask = command[1];
		break;
	}
}

void Sgb::onTransfer(unsigned char *frame) {
	unsigned char vram[4096];

	for (int i = 0; i < 256; i++) {
		const int y = i / 20;
		const int x = i % 20;
		unsigned char *src = frame + y * 8 * 160 + x * 8;
		unsigned char *dst = vram + 16 * i;

		for (int j = 0; j < 8; j++) {
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
		unsigned char *end = vram + 0x10000;
		unsigned char *src = vram;
		unsigned char *dst = spc_get_ram(spc);
		
		while (true) {
			if (src + 4 > end)
				break;
			
			int len = src[0] | src[1] << 8;
			int addr = src[2] | src[3] << 8;
			if (!len)
				break;

			src += 4;
			if ((src + len) > end || (addr + len) >= 0x10000)
				break;

			memcpy(dst + addr, src, len);
			src += len;
		}
		break;
	}
	case PAL_TRN:
		for (unsigned i = 0; i < 2048; i++)
			systemColors[i] = vram[i * 2] | vram[i * 2 + 1] << 8;

		break;
	case CHR_TRN & ~HIGH:
	{
		bool high = pending & HIGH;
		unsigned char *src = vram;
		unsigned char *dst = &tiles[(128 * high) * 64];
		for (unsigned n = 0; n < 128; n++) {
			for (unsigned y = 0; y < 8; y++) {
				unsigned a = src[0];
				unsigned b = src[1] << 1;
				unsigned c = src[16] << 2;
				unsigned d = src[17] << 3;
				for (unsigned x = 8; x > 0; --x) {
					dst[x] = (a & 1) | (b & 2) | (c & 4) | (d & 8);
					a >>= 1;
					b >>= 1;
					c >>= 1;
					d >>= 1;
				}
				dst += 8;
				src += 2;
			}
			src += 16;
		}
		break;
	}
	case PCT_TRN:
	{
		unsigned char *src = vram;
		memcpy(tilemap, src, sizeof tilemap);
		unsigned short *palettes = (unsigned short *)(src + (sizeof tilemap));
		unsigned short *dst = &colors[4 * 16];
		for (int i = 0; i < 64; i++)
			dst[i] = palettes[i];

		break;
	}
	case ATTR_TRN:
	{
		unsigned char *src = vram;
		unsigned char *dst = systemAttributes;
		for (unsigned n = 0; n < 45 * 90; n++) {
			unsigned char s = *src++;
			*dst++ = s >> 6 & 3;
			*dst++ = s >> 4 & 3;
			*dst++ = s >> 2 & 3;
			*dst++ = s >> 0 & 3;
		}
		break;
	}
	}

	pending = 0xFF;
}

void Sgb::refreshPalettes() {
	for (int i = 0; i < 16; i++)
		palette[i] = gbcToRgb32(colors[i * ((i & 3) != 0)]);
}

void Sgb::palnn(unsigned a, unsigned b) {
	if ((command[0] & 7) == 1) {
		unsigned short color[7];
		for (int i = 0; i < 7; i++)
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
}

void Sgb::attrBlk() {
	unsigned nset = command[1];
	if (!nset || nset >= 19)
		return;

	unsigned npacket = (nset * 6 + 16) / 16;
	if ((command[0] & 7) != npacket)
		return;
	
	for (unsigned i = 0; i < nset; i++) {
		unsigned ctrl = command[2 + i * 6 + 0] & 7;
		unsigned pals = command[2 + i * 6 + 1];
		unsigned x1 = command[2 + i * 6 + 2];
		unsigned y1 = command[2 + i * 6 + 3];
		unsigned x2 = command[2 + i * 6 + 4];
		unsigned y2 = command[2 + i * 6 + 5];

		unsigned inside = ctrl & 1;
		unsigned line = ctrl & 2;
		unsigned outside = ctrl & 4;

		unsigned insidepal = pals & 3;
		unsigned linepal = pals >> 2 & 3;
		unsigned outsidepal = pals >> 4 & 3;

		if (ctrl == 1) {
			ctrl = 3;
			linepal = insidepal;
		} else if (ctrl == 4) {
			ctrl = 6;
			linepal = outsidepal;
		}

		for (int j = 0; j < 20 * 18; j++) {
			unsigned x = j % 20;
			unsigned y = j / 20;

			if (outside && (x < x1 || x > x2 || y < y1 || y > y2)) {
				attributes[j] = outsidepal;
			} else if (inside && x > x1 && x < x2 && y > y1 && y < y2) {
				attributes[j] = insidepal;
			} else if (line)
				attributes[j] = linepal;
		}
	}
}

void Sgb::attrLin() {
	unsigned nset = command[1];
	if (!nset || nset >= 111)
		return;

	unsigned npacket = (nset + 17) / 16;
	if ((command[0] & 7) != npacket)
		return;

	for (unsigned i = 0; i < nset; i++) {
		unsigned char v = command[i + 2];
		unsigned line = v & 31;
		unsigned a = v >> 5 & 3;
		if (v & 0x80) { // horizontal
			if (line > 17)
				line = 17;

			memset(attributes + line * 20, a, 20);
		} else { // vertical
			if (line > 19)
				line = 19;

			unsigned char *dst = attributes + line;
			for (unsigned i = 0; i < 18; i++, dst += 20)
				dst[0] = a;
		}
	}
}

void Sgb::attrDiv() {
	if ((command[0] & 7) == 1) {
		unsigned char v = command[1];

		unsigned c = v & 3;
		unsigned a = v >> 2 & 3;
		unsigned b = v >> 4 & 3;

		unsigned pos = command[2];
		unsigned char *dst = attributes;
		if (v & 0x40) { // horizontal
			if (pos > 17)
				pos = 17;

			unsigned i;
			for (i = 0; i < pos; i++, dst += 20)
				memset(dst, a, 20);

			memset(dst, b, 20);
			dst += 20;
			for (i++; i < 18; i++, dst += 20)
				memset(dst, c, 20);
		} else { // vertical
			if (pos > 19)
				pos = 19;

			for (unsigned j = 0; j < 18; j++) {
				unsigned i;
				for (i = 0; i < pos; i++)
					*dst++ = a;

				*dst++ = b;
				for (i++; i < 20; i++)
					*dst++ = c;
			}
		}
	}
}

void Sgb::attrChr() {
	unsigned x = command[1];
	unsigned y = command[2];
	unsigned n = command[3] | command[4] << 8;
	if (n > 360)
		return;

	unsigned npacket = (n + 87) / 64;
	if ((command[0] & 7) < npacket)
		return;

	if (x > 19)
		x = 19;

	if (y > 17)
		y = 17;

	unsigned vertical = command[5];
	for (unsigned i = 0; i < n; i++) {
		unsigned char v = command[i / 4 + 6];
		unsigned a = v >> (2 * (3 - (i & 3))) & 3;
		attributes[y * 20 + x] = a;
		if (vertical) {
			y++;
			if (y == 18) {
				y = 0;
				x++;
				if (x == 20)
					return;
			}
		} else {
			x++;
			if (x == 20) {
				x = 0;
				y++;
				if (y == 18)
					return;
			}
		}
	}
}

void Sgb::attrSet() {
	if ((command[0] & 7) == 1) {
		unsigned attr = command[1] & 0x3F;
		if (attr >= 45)
			attr = 44;

		memcpy(attributes, &systemAttributes[attr * 20 * 18], sizeof attributes);
		if (command[1] & 0x40)
			mask = 0;
	}
}

void Sgb::palSet() {
	if ((command[0] & 7) == 1) {
		unsigned p0 = command[1] | ((command[2] << 8) & 0x100);
		for (unsigned i = 0; i < 4; i++) {
			unsigned p = command[1 + i * 2] | ((command[1 + i * 2 + 1] << 8) & 0x100);
			colors[0 * 4 + 0] = systemColors[p0 * 4 + 0];
			colors[i * 4 + 1] = systemColors[p  * 4 + 1];
			colors[i * 4 + 2] = systemColors[p  * 4 + 2];
			colors[i * 4 + 3] = systemColors[p  * 4 + 3];
		}
		if (command[9] & 0x80) {
			unsigned attr = command[9] & 0x3F;
			if (attr >= 45)
				attr = 44;
			
			memcpy(attributes, &systemAttributes[attr * 20 * 18], sizeof attributes);
		}
		if (command[9] & 0x40)
			mask = 0;

		refreshPalettes();
	}
}

void Sgb::cmdSound() {
	if ((command[0] & 7) == 1) {
		soundControl[1] = command[1];
		soundControl[2] = command[2];
		soundControl[3] = command[3];
		soundControl[0] = command[4];
	}
}

unsigned Sgb::generateSamples(int16_t *soundBuf, uint64_t &samples) {
	if (!soundBuf)
		return -1;

	short buf[4096];
	unsigned diff = samples - lastUpdate_;
	samples = diff / 65;
	lastUpdate_ += samples * 65;
	spc_set_output(spc, buf, 2048);
	bool matched = true;
	for (unsigned p = 0; p < 4; p++) {
		if (spc_read_port(spc, 0, p) != soundControl[p])
			matched = false;
	}
	if (matched) {
		soundControl[0] = 0;
		soundControl[1] = 0;
		soundControl[2] = 0;
	}
	for (unsigned p = 0; p < 4; p++)
		spc_write_port(spc, 0, p, soundControl[p]);

	spc_end_frame(spc, samples * 32);
	memcpy(soundBuf, buf, sizeof buf);
	return diff % 65;
}

SYNCFUNC(Sgb) {
	NSS(cgbColorsRgb32_);

	NSS(transfer);
	NSS(packet);
	NSS(command);
	NSS(commandIndex);

	NSS(joypadIndex);
	NSS(joypadMask);

	NSS(systemColors);
	NSS(colors);
	NSS(palette);
	NSS(attributes);

	NSS(pending);
	NSS(pendingCount);
	NSS(mask);
}

}
