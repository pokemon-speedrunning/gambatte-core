//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#include "gambatte.h"
#include "cinterface.h"

#include <cstring>

namespace gambatte {

typedef struct {
	GB gb;
	gambatte::uint_least32_t vBuf[160 * 144];
	gambatte::uint_least32_t sBuf[35112 + 2064];
	gambatte::uint_least32_t rand = 774533096;
} gbState_;

static unsigned randInputGetter(void *gbs) {
	gbState_ *gbState = (gbState_ *)gbs;
	gbState->rand *= 1103515245;
	gbState->rand += 12345;
	gbState->rand &= 0xFFFFFFFF;
	return (gbState->rand >> 24) & 0xFF;
}

#define BASE_LOAD_FLAGS GB::LoadFlag::MULTICART_COMPAT | GB::LoadFlag::READONLY_SAV

void test(gbState_ *gbState, std::string const rom, char const *state, std::size_t size) {
	gbState->gb.setInputGetter(randInputGetter, gbState);
	if (gbState->gb.load(rom, BASE_LOAD_FLAGS))
		return;

	if (gbState->gb.loadBios("roms/bios.gb"))
		return;

	if (!gbState->gb.loadState(state, size)) {
		gbState->gb.load(rom, GB::LoadFlag::CGB_MODE | BASE_LOAD_FLAGS);
		if (gbState->gb.loadBios("roms/bios.gbc"))
			return;

		if (!gbState->gb.loadState(state, size)) {
			gbState->gb.load(rom, GB::LoadFlag::SGB_MODE | BASE_LOAD_FLAGS);
			if (gbState->gb.loadBios("roms/bios.sgb"))
				return;

			if (!gbState->gb.loadState(state, size))
				return;
		}
	}

	std::size_t samplesRan = 0;
	while (samplesRan < 35112 * 4) {
		std::size_t samplesToRun = 35112;
		gbState->gb.runFor(gbState->vBuf, 160, gbState->sBuf, samplesToRun);
		samplesRan += samplesToRun;
	}
}

GBEXPORT int LLVMFuzzerTestOneInput(unsigned char const *state, size_t size) {
	gbState_ *gbState = new gbState_;
	test(gbState, "roms/Pokemon - Gold Version (UE) [C][!].gbc", (char const *)state, size);
	test(gbState, "roms/X - Xekkusu (J) [!].gb", (char const *)state, size);
	test(gbState, "roms/Hugo (E) [S].gb", (char const *)state, size);
	test(gbState, "roms/Kanji Boy 2 (J) [C][!].gbc", (char const *)state, size);
	test(gbState, "roms/Gameboy Camera Gold - Zelda Edition (U) [S].gb", (char const *)state, size);
	test(gbState, "roms/Robopon - Sun Version (U) [C][!].gbc", (char const *)state, size);
	test(gbState, "roms/Pokemon Card GB (J) [C][!].gbc", (char const *)state, size);
	test(gbState, "roms/Tetris (W) (V1.0) [!].gb", (char const *)state, size);
	test(gbState, "roms/Bomberman Collection (J) [S].gb", (char const *)state, size);
	delete gbState;
	return 0;
}

}
