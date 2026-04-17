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

namespace {

void saveStateCallback(unsigned char **o, void *i, std::size_t len) {
	std::memcpy(*o, i, len);
	*o += len;
}

void loadStateCallback(unsigned char **i, void *o, std::size_t len) {
	std::memcpy(o, *i, len);
	*i += len;
}

}

namespace gambatte {

Sgb::Sgb()
: transfer(0xFF)
, commandIndex(0)
, disableCommands(false)
, headerSent(false)
, joypadIndex(0)
, joypadMask(0)
, videoBuf_(0)
, pitch_(0)
, borderFade(0)
, pending(0xFF)
, pendingCount(0)
, mask(0)
, samplesAccumulated_(0)
{
	// FIXME: this code is ugly
	unsigned i = 0;
	for (unsigned b = 0; b < 32; b++) {
		for (unsigned g = 0; g < 32; g++) {
			for (unsigned r = 0; r < 32; r++)
				cgbColorsRgb32_[i++] = ((b * 0xFF + 0xF) / 0x1F) | (((g * 0xFF + 0xF) / 0x1F) << 8) | (((r * 0xFF + 0xF) / 0x1F) << 16) | 0xFF << 24;
		}
	}

	std::memset(packet, 0, sizeof packet);
	std::memset(command, 0, sizeof command);

	std::memset(logoBuf, 0, sizeof logoBuf);
	std::memset(hookBuf, 0, sizeof hookBuf);

	std::memset(frameBuf_, 0, sizeof frameBuf_);

	std::memset(systemColors, 0, sizeof systemColors);
	std::memset(colors, 0, sizeof colors);
	std::memset(systemAttributes, 0, sizeof systemAttributes);
	std::memset(attributes, 0, sizeof attributes);

	std::memset(systemTiles, 0, sizeof systemTiles);
	std::memset(tiles, 0, sizeof tiles);
	std::memset(systemTilemap, 0, sizeof systemTilemap);
	std::memset(tilemap, 0, sizeof tilemap);
	std::memset(systemTileColors, 0, sizeof systemTileColors);
	std::memset(tileColors, 0, sizeof tileColors);

	std::memset(spcState, 0, sizeof spcState);
	std::memset(soundControl, 0, sizeof soundControl);

	refreshPalettes();
	spc.init();
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

void Sgb::setStatePtrs(SaveState &state, bool sgb) {
	if (sgb) {
		state.mem.sgb.packet.set(packet, sizeof packet);
		state.mem.sgb.command.set(command, sizeof command);
		state.mem.sgb.logoBuf.set(logoBuf, sizeof logoBuf);
		state.mem.sgb.hookBuf.set(hookBuf, sizeof hookBuf);
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
	} else {
		state.mem.sgb.packet.set(packet, 1);
		state.mem.sgb.command.set(command, 1);
		state.mem.sgb.logoBuf.set(logoBuf, 1);
		state.mem.sgb.hookBuf.set(hookBuf, 1);
		state.mem.sgb.frameBuf.set(frameBuf_, 1);
		state.mem.sgb.systemColors.set(systemColors, 1);
		state.mem.sgb.colors.set(colors, 1);
		state.mem.sgb.systemAttributes.set(systemAttributes, 1);
		state.mem.sgb.attributes.set(attributes, 1);
		state.mem.sgb.systemTiles.set(systemTiles, 1);
		state.mem.sgb.tiles.set(tiles, 1);
		state.mem.sgb.systemTilemap.set(systemTilemap, 1);
		state.mem.sgb.tilemap.set(tilemap, 1);
		state.mem.sgb.systemTileColors.set(systemTileColors, 1);
		state.mem.sgb.tileColors.set(tileColors, 1);
		state.mem.sgb.spcState.set(spcState, 1);
		state.mem.sgb.soundControl.set(soundControl, 1);
	}
}

void Sgb::saveState(SaveState &state) const {
	state.mem.sgb.transfer = transfer;
	state.mem.sgb.commandIndex = commandIndex;
	state.mem.sgb.disableCommands = disableCommands;
	state.mem.sgb.headerSent = headerSent;
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
	disableCommands = state.mem.sgb.disableCommands;
	headerSent = state.mem.sgb.headerSent;
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
	spc.copy_state(&o, saveStateCallback);
}

void Sgb::loadSpcState() {
	spc.set_output(0, 0);
	unsigned char *i = spcState;
	spc.copy_state(&i, loadStateCallback);
}

void Sgb::onJoypad(unsigned oldJoypad, unsigned newJoypad) {
	handleTransfer(oldJoypad, newJoypad);

	if ((oldJoypad & 0x20) == 0 && (newJoypad & 0x20) != 0)
		joypadIndex = (joypadIndex + 1) & joypadMask;
}

void Sgb::updateScreen(bool blanklcd) {
	unsigned char frame[160 * 144];

	for (unsigned y = 0; y < 144; y++) {
		for (unsigned x = 0; x < 160; x++)
			frame[y * 160 + x] = 3 - (videoBuf_[y * pitch_ + x] >> 4 & 3);
	}

	if (pending != 0xFF && --pendingCount == 0)
		onTransfer(frame);

	if (!mask && !blanklcd)
		std::memcpy(frameBuf_, frame, sizeof frame);

	// Border data can affect the GB area, so we need to go through that data too
	uint_least32_t frame_[256 * 224];
	uint_least32_t *gbFrame = &frame_[40 * 256 + 48];

	switch (mask) {
	case 0:
	case 1:
		for (unsigned y = 0; y < 144; y++) {
			for (unsigned x = 0; x < 160; x++) {
				unsigned attribute = attributes[(y / 8) * 20 + (x / 8)] & 3;
				gbFrame[y * 256 + x] = palette[attribute * 4 + frameBuf_[y * 160 + x]];
			}
		}

		break;
	case 2:
	{
		unsigned long const black = gbcToRgb32(0);
		for (unsigned y = 0; y < 144; y++) {
			for (unsigned x = 0; x < 160; x++)
				gbFrame[y * 256 + x] = black;
		}

		break;
	}
	case 3:
	{
		unsigned long const pal0 = palette[0];
		for (unsigned y = 0; y < 144; y++) {
			for (unsigned x = 0; x < 160; x++)
				gbFrame[y * 256 + x] = pal0;
		}

		break;
	}
	}

	if (borderFade && --borderFade == 72) {
		std::memcpy(systemTiles,      tiles,      sizeof tiles);
		std::memcpy(systemTilemap,    tilemap,    sizeof tilemap);
		std::memcpy(systemTileColors, tileColors, sizeof tileColors);
	}

	unsigned long colors[16 * 4];
	if (borderFade == 0) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i]);
	} else if (borderFade > 78) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], (142 - borderFade) / 2);
	} else if (borderFade < 64) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], borderFade / 2);
	} else {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(0);
	}

	for (unsigned tileY = 5; tileY < 23; tileY++) {
		for (unsigned tileX = 6; tileX < 26; tileX++) {
			unsigned short tile = systemTilemap[tileY * 32 + tileX];
			if (borderFade > 68 && borderFade <= 76)
				tile = 0;

			if (tile & 0x300)
				continue;

			unsigned char flipX = (tile & 0x4000) ? 0 : 7;
			unsigned char flipY = (tile & 0x8000) ? 7 : 0;
			unsigned char pal = (tile >> 10) & 3;
			for (unsigned y = 0; y < 8; y++) {
				unsigned base = (tile & 0xFF) * 32 + (y ^ flipY) * 2;
				for (unsigned x = 0; x < 8; x++) {
					unsigned char bit = 1 << (x ^ flipX);
					unsigned char color =
					((systemTiles[base + 0x00] & bit) ? 1 : 0) |
					((systemTiles[base + 0x01] & bit) ? 2 : 0) |
					((systemTiles[base + 0x10] & bit) ? 4 : 0) |
					((systemTiles[base + 0x11] & bit) ? 8 : 0);

					uint_least32_t *pixel = &frame_[(tileY * 8 + y) * 256 + tileX * 8 + x];
					if (color)
						*pixel = colors[pal * 16 + color];
				}
			}
		}
	}

	for (unsigned y = 0; y < 144; y++) {
		for (unsigned x = 0; x < 160; x++)
			videoBuf_[y * pitch_ + x] = gbFrame[y * 256 + x];
	}
}

unsigned Sgb::updateScreenBorder(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
	if (!videoBuf_ || !videoBuf)
		return -1;

	uint_least32_t frame[256 * 224];
	std::memset(frame, 0, sizeof frame);

	unsigned long colors[16 * 4];
	if (borderFade == 0) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i]);
	} else if (borderFade > 78) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], (142 - borderFade) / 2);
	} else if (borderFade < 64) {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(systemTileColors[i], borderFade / 2);
	} else {
		for (unsigned i = 0; i < 16 * 4; i++)
			colors[i] = gbcToRgb32(0);
	}

	uint_least32_t *gbFrame = &frame[40 * 256 + 48];
	for (unsigned y = 0; y < 144; y++) {
		for (unsigned x = 0; x < 160; x++)
			gbFrame[y * 256 + x] = videoBuf_[y * pitch_ + x];
	}

	for (unsigned tileY = 0; tileY < 28; tileY++) {
		bool inGbFrameY = tileY >= 5 && tileY < 23;
		for (unsigned tileX = 0; tileX < 32; tileX++) {
			// Skip the GB area
			if (tileX == 6 && inGbFrameY) {
				tileX = 25;
				continue;
			}

			unsigned short tile = systemTilemap[tileY * 32 + tileX];
			if (borderFade > 68 && borderFade <= 76)
				tile = 1;

			if (tile & 0x300)
				continue;

			unsigned char flipX = (tile & 0x4000) ? 0 : 7;
			unsigned char flipY = (tile & 0x8000) ? 7 : 0;
			unsigned char pal = (tile >> 10) & 3;
			for (unsigned y = 0; y < 8; y++) {
				unsigned base = (tile & 0xFF) * 32 + (y ^ flipY) * 2;
				for (unsigned x = 0; x < 8; x++) {
					unsigned char bit = 1 << (x ^ flipX);
					unsigned char color =
					((systemTiles[base + 0x00] & bit) ? 1 : 0) |
					((systemTiles[base + 0x01] & bit) ? 2 : 0) |
					((systemTiles[base + 0x10] & bit) ? 4 : 0) |
					((systemTiles[base + 0x11] & bit) ? 8 : 0);

					uint_least32_t *pixel = &frame[(tileY * 8 + y) * 256 + tileX * 8 + x];
					if (color == 0)
						*pixel = palette[0];
					else
						*pixel = colors[pal * 16 + color];
				}
			}
		}
	}

	for (unsigned y = 0; y < 224; y++) {
		for (unsigned x = 0; x < 256; x++)
			videoBuf[y * pitch + x] = frame[y * 256 + x];
	}

	return 0;
}

void Sgb::handleTransfer(unsigned oldJoypad, unsigned newJoypad) {
	if (disableCommands)
		return;

	// 0x00 in the joypad holds the packet transfer in a reset state
	if ((newJoypad & 0x30) == 0) {
		std::memset(packet, 0, sizeof packet);
		transfer = 0xFF;
		return;
	}

	// commands are only "committed" with a 0x30 write after them
	if ((newJoypad & 0x30) != 0x30)
		return;

	// 0x00 -> 0x30 begins a packet transfer
	if (transfer == 0xFF && (oldJoypad & 0x30) == 0) {
		transfer = 0;
		return;
	}

	// 0x10/0x20 -> 0x30 writes a bit, if a packet transfer is active
	if (transfer != 0xFF) {
		if (transfer < 128) {
			// 0x10 -> 0x30 writes a 1 bit
			// 0x20 -> 0x30 writes a 0 bit
			packet[transfer >> 3] |= ((oldJoypad & 0x30) == 0x10) << (transfer & 7);
			transfer++;
		} else {
			transfer = 0xFF;
			std::memcpy(command + commandIndex * sizeof packet, packet, sizeof packet);

			if (headerSent) {
				unsigned commandLength = command[0] & 7;
				if (commandLength == 0)
					commandLength = 1;

				if (++commandIndex == commandLength) {
					onCommand();
					commandIndex = 0;
				}

				if ((command[0] >> 3) >= 0x1E)
					commandIndex = 0;
			} else {
				// The SGB bootrom sends over the header using special commands
				if (command[0] != 0xF1) {
					commandIndex = 0;
					return;
				}

				// Command "0xF1" is hardcoded to a length of 6 packets
				if (++commandIndex == 6) {
					// The GB logo is copied to a buffer and validated by the SGB BIOS
					// If the logo is invalid, it's ignored by this implementation
					std::memcpy(&logoBuf[0x00], &command[0x02], 0xE);
					std::memcpy(&logoBuf[0x0E], &command[0x12], 0xE);
					std::memcpy(&logoBuf[0x1C], &command[0x22], 0xE);
					std::memcpy(&logoBuf[0x2A], &command[0x32], 0x6);

					// Each packet has a checksum attached to it
					// Normally the SGB BIOS validates these checksums
					// This implementation does nothing if they're invalid however

					// GB header must have 0x03 in the SGB flag and 0x33 in the old licensee code
					// Otherwise, SGB commands are disabled
					if (command[0x4C] != 0x03 || command[0x53] != 0x33)
						disableCommands = true;

					headerSent = true;
					commandIndex = 0;
				}
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
	case SOUND:
		cmdSound();
		break;
	case SOU_TRN:
		pending = SOU_TRN;
		pendingCount = 3;
		break;
	case PAL_SET:
		palSet();
		break;
	case PAL_TRN:
		pending = PAL_TRN;
		pendingCount = 3;
		break;
	case ATRC_EN:
		// This allows for disabling the SGB screensaver
		// In an HLE context, this is irrelevant
		break;
	case TEST_EN:
		// This is stubbed on all retail SGB releases
		break;
	case ICON_EN:
		// Allows for disabling certain SGB features
		// Bit 0 disables built-in SGB palettes (user selected)
		// Bit 1 disables controller mode options
		// Bit 2 disables SGB commands
		// In an HLE context, only bit 2 bit is relevant
		disableCommands = command[1] & 4;
		break;
	case DATA_SND:
		dataSnd();
		break;
	case DATA_TRN:
		pending = DATA_TRN;
		pendingCount = 3;
		break;
	case MLT_REQ:
		joypadMask = command[1] & 3;
		joypadIndex += joypadMask == 2;
		joypadIndex &= joypadMask;
		break;
	case JUMP:
		// Not relevant in an HLE context
		break;
	case CHR_TRN:
		pending = CHR_TRN;
		if (command[1] & 1)
			pending |= HIGH;

		pendingCount = 3;
		break;
	case PCT_TRN:
		pending = PCT_TRN;
		pendingCount = 3;
		break;
	case ATTR_TRN:
		pending = ATTR_TRN;
		pendingCount = 3;
		break;
	case ATTR_SET:
		attrSet();
		break;
	case MASK_EN:
		mask = command[1] & 3;
		break;
	case OBJ_TRN:
		// This is stubbed on all retail SGB releases
		break;
	case PAL_PRI:
		// This allows for disabling user set palettes
		// In an HLE context, this is irrelevant
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
		unsigned char *end = &vram[sizeof vram - 1];
		unsigned char *src = vram;
		unsigned char *dst = spc.get_ram();

		// FIXME: Not accurate, the SGB does not do bound checking
		// If src goes past vram, it just reads memory past it
		// If it goes far enough (overflows lower 16 bits), the ptr is set to the beginning of the SNES ROM
		// dst just wraps around if it overflows (SPC has a 16 bit address space)
		while (true) {
			if (src + 4 > end)
				break;

			unsigned len = src[0] | src[1] << 8;
			unsigned addr = src[2] | src[3] << 8;
			// FIXME: The first iteration does not 0-terminate the loop
			// Instead len gets treated as 0x10000 if it's 0
			// This would overflow anyways, but this is not bound checked in reality
			if (!len)
				break;

			src += 4;
			if ((src + len) > end || (addr + len) >= 0x10000)
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
	case DATA_TRN:
	{
		// This doesn't make sense in an HLE context
		// TODO: Maybe could allow writing in some areas?
		break;
	}
	case CHR_TRN | HIGH:
	case CHR_TRN:
	{
		unsigned char *tileAddr = (pending & HIGH) ? &tiles[4096] : &tiles[0];
		std::memcpy(tileAddr, vram, 4096);
		break;
	}
	case PCT_TRN:
	{
		unsigned char *src = vram;
		unsigned short *dst = tilemap;
		for (unsigned i = 0; i < 32 * 32; i++)
			dst[i] = src[i * 2] | src[i * 2 + 1] << 8;

		src += sizeof tilemap;
		dst = tileColors;
		for (unsigned i = 0; i < 16 * 4; i++)
			dst[i] = src[i * 2] | src[i * 2 + 1] << 8;

		borderFade = 143;
		break;
	}
	case ATTR_TRN:
	{
		std::memcpy(systemAttributes, vram, sizeof systemAttributes);
		break;
	}
	}

	pending = 0xFF;
}

void Sgb::refreshPalettes() {
	for (unsigned i = 0; i < 16; i++)
		palette[i] = gbcToRgb32(colors[i * ((i & 3) != 0)]);
}

unsigned char Sgb::readCmdBuf(unsigned offset) {
	// Only 7 packets can be sent to the command buffer
	// However, corrupt commands can read past this

	if (offset < sizeof command)
		return command[offset];

	offset -= sizeof command;

	// Next 0x10 bytes after the command buffer is 0 filled
	// This is probably some padding for the command buffer
	if (offset < 0x10)
		return 0;

	offset -= 0x10;

	// Next 0x30 bytes has a buffer for the GB logo sent
	if (offset < 0x30)
		return logoBuf[offset];

	offset -= 0x30;

	// Next 0x50 bytes appear to be 0 filled?
	// Next 0x100 bytes after that is used in the user drawing part of the SGB BIOS
	// Assuming that part is never used, it remains 0 filled too

	if (offset < 0x150)
		return 0;

	offset -= 0x150;

	// Next 0x400 bytes are used for various hooks
	// These hooks can be set by the game with DATA_SND
	// Most games place the same hooks (bugfix patch)
	// SGB Rev B + SGB2 automatically write in the common bugfix patch

	if (offset < sizeof hookBuf)
		return hookBuf[offset];

	offset -= sizeof hookBuf;

	// Next 2 bytes are used for SGB BIOS menu choices
	// These are constant assuming the user only plays the game

	if (offset == 0)
		return 0;
	if (offset == 1)
		return 3;

	// The rest of memory is unreachable
	return 0;
}

void Sgb::writeAttrRect(unsigned x, unsigned y, unsigned width, unsigned height, unsigned char pal) {
	unsigned attrIndex = y * 20 + x;
	if (width == 0)
		width = 0x100;
	if (height == 0)
		height = 0x100;

	for (unsigned yOffset = 0; yOffset < height; yOffset++) {
		for (unsigned xOffset = 0; xOffset < width; xOffset++) {
			if (attrIndex + xOffset >= sizeof attributes)
				return;

			attributes[attrIndex + xOffset] = pal;
		}

		attrIndex += 20;
	}
}

void Sgb::setAttrFile(unsigned attr) {
	unsigned attrFileIndex = attr * 90;
	unsigned attrIndex = 0;
	for (unsigned i = 0; i < 90; i++) {
		// FIXME: This isn't quite accurate
		// Past the attribute files buffer is a buffer for the GB framebuffer
		// Emulating this might be as simple as using frameBuf_?
		unsigned char b = 0;
		if (attrFileIndex < sizeof systemAttributes)
			b = systemAttributes[attrFileIndex];

		attributes[attrIndex++] = b >> 6;
		attributes[attrIndex++] = (b >> 4) & 3;
		attributes[attrIndex++] = (b >> 2) & 3;
		attributes[attrIndex++] = b & 3;

		attrFileIndex++;
	}
}

void Sgb::palnn(unsigned a, unsigned b) {
	unsigned short color[7];
	for (unsigned i = 0; i < 7; i++)
		color[i] = command[1 + i * 2] | command[1 + i * 2 + 1] << 8;

	colors[0 * 4 + 0] = color[0];
	colors[a * 4 + 1] = color[1];
	colors[a * 4 + 2] = color[2];
	colors[a * 4 + 3] = color[3];
	colors[b * 4 + 1] = color[4];
	colors[b * 4 + 2] = color[5];
	colors[b * 4 + 3] = color[6];

	refreshPalettes();
}

void Sgb::attrBlk() {
	unsigned nset = command[1] & 0x1F;
	if (nset == 0)
		nset = 0x100;

	for (unsigned i = 0; i < nset; i++) {
		unsigned ctrl = readCmdBuf(2 + i * 6 + 0) & 7;
		unsigned pals = readCmdBuf(2 + i * 6 + 1);
		unsigned x1 = readCmdBuf(2 + i * 6 + 2) & 0x1F;
		unsigned y1 = readCmdBuf(2 + i * 6 + 3) & 0x1F;
		unsigned x2 = readCmdBuf(2 + i * 6 + 4) & 0x1F;
		unsigned y2 = readCmdBuf(2 + i * 6 + 5) & 0x1F;

		bool inside = ctrl & 1;
		bool line = ctrl & 2;
		bool outside = ctrl & 4;

		unsigned char insidepal = pals & 3;
		unsigned char linepal = pals >> 2 & 3;
		unsigned char outsidepal = pals >> 4 & 3;

		if (ctrl == 1) {
			line = true;
			linepal = insidepal;
		} else if (ctrl == 4) {
			line = true;
			linepal = outsidepal;
		}

		if (inside)
			writeAttrRect(x1 + 1, y1 + 1, (x2 - x1 - 1) & 0xFF, (y2 - y1 - 1) & 0xFF, insidepal);

		if (outside) {
			if (y1 != 0)
				writeAttrRect(0, 0, 20, y1, outsidepal);

			if (y2 < 17)
				writeAttrRect(0, y2 + 1, 20, 17 - y2, outsidepal);

			if (x1 != 0)
				writeAttrRect(0, y1, x1, (y2 - y1 + 1) & 0xFF, outsidepal);

			if (x2 < 19)
				writeAttrRect(x2 + 1, y1, 19 - x2, (y2 - y1 + 1) & 0xFF, outsidepal);
		}

		if (line) {
			writeAttrRect(x1, y1, (x2 - x1 + 1) & 0xFF, 1, linepal);
			writeAttrRect(x1, y2, (x2 - x1 + 1) & 0xFF, 1, linepal);
			writeAttrRect(x1, y1, 1, (y2 - y1 + 1) & 0xFF, linepal);
			writeAttrRect(x2, y1, 1, (y2 - y1 + 1) & 0xFF, linepal);
		}
	}
}

void Sgb::attrLin() {
	unsigned nset = command[1];
	if (nset == 0)
		nset = 0x100;

	for (unsigned i = 0; i < nset; i++) {
		unsigned char v = readCmdBuf(i + 2);
		unsigned line = v & 0x1F;
		unsigned pal = v >> 5 & 3;
		if (v & 0x80) // horizontal
			writeAttrRect(0, line, 20, 1, pal);
		else // vertical
			writeAttrRect(line, 0, 1, 18, pal);
	}
}

void Sgb::attrDiv() {
	unsigned char v = command[1];

	unsigned char highpal = v & 3;
	unsigned char lowpal = v >> 2 & 3;
	unsigned char linepal = v >> 4 & 3;

	unsigned pos = command[2] & 0x1F;

	std::memset(attributes, lowpal, sizeof attributes);

	if (v & 0x40) { // horizontal
		writeAttrRect(0, pos, 20, 1, linepal);
		if (pos < 17)
			writeAttrRect(0, pos + 1, 20, 17 - pos, highpal);
	} else { // vertical
		writeAttrRect(pos, 0, 1, 18, linepal);
		if (pos < 19)
			writeAttrRect(pos + 1, 0, 19 - pos, 18, highpal);
	}
}

void Sgb::attrChr() {
	unsigned x = command[1] & 0x1F;
	unsigned y = command[2] & 0x1F;
	unsigned nset = (command[3] | command[4] << 8) & 0x1FF;
	if (nset == 0)
		nset = 0x10000;

	unsigned vertical = command[5] & 1;

	for (unsigned i = 0; i < nset; i++) {
		unsigned char v = readCmdBuf(i / 4 + 6);
		unsigned char pal = (v >> (((~i) & 3) << 1)) & 3;
		unsigned attrIndex = y * 20 + x;
		if (attrIndex < sizeof attributes)
			attributes[attrIndex] = pal;

		if (vertical) {
			y = (y + 1) & 0xFF;
			if (y == 18) {
				y = 0;
				x = (x + 1) & 0xFF;
				if (x == 20)
					break;
			}
		} else {
			x = (x + 1) & 0xFF;
			if (x == 20) {
				x = 0;
				y = (y + 1) & 0xFF;
				if (y == 18)
					break;
			}
		}
	}
}

void Sgb::attrSet() {
	unsigned attr = command[1] & 0x3F;
	setAttrFile(attr);

	if (command[1] & 0x40)
		mask = 0;
}

void Sgb::palSet() {
	for (unsigned i = 0; i < 4; i++) {
		unsigned p = (command[1 + i * 2] | (command[1 + i * 2 + 1] << 8)) & 0x1FF;
		colors[i * 4 + 0] = systemColors[p * 4 + 0];
		colors[i * 4 + 1] = systemColors[p * 4 + 1];
		colors[i * 4 + 2] = systemColors[p * 4 + 2];
		colors[i * 4 + 3] = systemColors[p * 4 + 3];
	}

	if (command[9] & 0x80) {
		unsigned attr = command[9] & 0x3F;
		setAttrFile(attr);
	}

	if (command[9] & 0x40)
		mask = 0;

	refreshPalettes();
}

void Sgb::dataSnd() {
	unsigned addr = command[1] << 8 | command[2];
	unsigned bank = command[3];
	unsigned len = command[4];
	if (len == 0)
		len = 0x100;

	for (unsigned i = 0; i < len; i++) {
		// These banks can address SNES WRAM
		if (bank <= 0x3F || bank == 0x7E || (bank >= 0x80 && bank <= 0xBF)) {
			// SGB hook area
			if (addr >= 0x800 && addr <= 0xBFF) {
				hookBuf[addr - 0x800] = readCmdBuf(i + 5);
			}

			// TODO: Allow writing elsewhere?
		}

		addr++;
		if (addr == 0x10000) {
			addr = 0;
			bank = (bank + 1) & 0xFF;
		}
	}
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
	spc.set_output(buf, 2048);

	bool matched = true;
	for (unsigned p = 0; p < 4; p++) {
		if (spc.read_port(0, p) != soundControl[p])
			matched = false;
	}

	if (matched) {
		soundControl[0] = 0;
		soundControl[1] = 0;
		soundControl[2] = 0;
	}

	for (unsigned p = 0; p < 4; p++)
		spc.write_port(0, p, soundControl[p]);

	spc.end_frame(samples * 32);
	if (soundBuf)
		std::memcpy(soundBuf, buf, sizeof buf);

	return samplesAccumulated_;
}

SYNCFUNC(Sgb) {
	NSS(transfer);
	NSS(packet);
	NSS(command);
	NSS(commandIndex);

	NSS(disableCommands);
	NSS(headerSent);

	NSS(logoBuf);
	NSS(hookBuf);

	NSS(joypadIndex);
	NSS(joypadMask);

	NSS(frameBuf_);

	NSS(systemColors);
	NSS(colors);
	refreshPalettes();
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
