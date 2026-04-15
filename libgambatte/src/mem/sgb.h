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

#ifndef SGB_H
#define SGB_H

#include "gbint.h"
#include "newstate.h"
#include "snes_spc/SNES_SPC.h"

#include <cstddef>

namespace gambatte {

struct SaveState;

class Sgb {
public:
	Sgb();
	void setStatePtrs(SaveState &state, bool sgb);
	void saveState(SaveState &state) const;
	void loadState(SaveState const &state);

	unsigned getJoypadIndex() const { return joypadIndex; }

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		videoBuf_ = videoBuf;
		pitch_ = pitch;
	}

	void accumulateSamples(std::size_t samples) { samplesAccumulated_ = (samplesAccumulated_ + samples) % (35112 + 2064); }
	unsigned generateSamples(short *soundBuf, std::size_t &samples);

	void setCgbPalette(unsigned *lut) {
		for (int i = 0; i < 32768; i++)
			cgbColorsRgb32_[i] = lut[i];

		refreshPalettes();
	}

	void onJoypad(unsigned oldJoypad, unsigned newJoypad);
	void updateScreen(bool blanklcd);
	unsigned updateScreenBorder(uint_least32_t *videoBuf, std::ptrdiff_t pitch);

	template<bool isReader>void SyncState(NewState *ns);

private:
	unsigned long cgbColorsRgb32_[32768];

	unsigned char transfer;
	unsigned char packet[0x10];
	unsigned char command[0x10 * 7];
	unsigned char commandIndex;

	bool disableCommands;
	bool headerSent;

	unsigned char logoBuf[0x30];
	unsigned char hookBuf[0x400];

	unsigned char joypadIndex;
	unsigned char joypadMask;

	uint_least32_t *videoBuf_;
	std::ptrdiff_t pitch_;
	unsigned char frameBuf_[160 * 144];

	unsigned short systemColors[512 * 4];
	unsigned short colors[4 * 4];
	unsigned long palette[4 * 4];
	// Only the first 45 * 90 bytes have valid attributes
	unsigned char systemAttributes[0x1000];
	unsigned char attributes[20 * 18];

	unsigned char systemTiles[256 * 8 * 4];
	unsigned char tiles[256 * 8 * 4];
	unsigned short systemTilemap[32 * 32];
	unsigned short tilemap[32 * 32];
	unsigned short systemTileColors[16 * 4];
	unsigned short tileColors[16 * 4];
	unsigned char borderFade;

	unsigned char pending;
	unsigned char pendingCount;
	unsigned char mask;

	SNES_SPC spc;
	unsigned char spcState[SNES_SPC::state_size];
	unsigned char soundControl[4];
	unsigned long samplesAccumulated_;

	enum Command {
		PAL01    = 0x00,
		PAL23    = 0x01,
		PAL03    = 0x02,
		PAL12    = 0x03,
		ATTR_BLK = 0x04,
		ATTR_LIN = 0x05,
		ATTR_DIV = 0x06,
		ATTR_CHR = 0x07,
		SOUND    = 0x08,
		SOU_TRN  = 0x09,
		PAL_SET  = 0x0A,
		PAL_TRN  = 0x0B,
		ATRC_EN  = 0x0C,
		TEST_EN  = 0x0D,
		ICON_EN  = 0x0E,
		DATA_SND = 0x0F,
		DATA_TRN = 0x10,
		MLT_REQ  = 0x11,
		JUMP     = 0x12,
		CHR_TRN  = 0x13,
		PCT_TRN  = 0x14,
		ATTR_TRN = 0x15,
		ATTR_SET = 0x16,
		MASK_EN  = 0x17,
		OBJ_TRN  = 0x18,
		PAL_PRI  = 0x19,
		HIGH     = 0x80 // for CHR_TRN 
	};

	void saveSpcState();
	void loadSpcState();

	void handleTransfer(unsigned oldJoypad, unsigned newJoypad);
	void onCommand();
	void onTransfer(unsigned char *frame);

	unsigned long gbcToRgb32(unsigned bgr15);
	unsigned long gbcToRgb32(unsigned bgr15, unsigned fade);
	void refreshPalettes();

	unsigned char readCmdBuf(unsigned offset);
	void writeAttrRect(unsigned x, unsigned y, unsigned width, unsigned height, unsigned char pal);
	void setAttrFile(unsigned attr);

	void palnn(unsigned a, unsigned b);
	void attrBlk();
	void attrLin();
	void attrDiv();
	void attrChr();
	void attrSet();
	void palSet();
	void dataSnd();
	void cmdSound();
};

}

#endif
