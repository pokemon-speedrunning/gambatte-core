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
#include "bess.h"
#include "counterdef.h"
#include "savestate.h"
#include "array.h"

#include <sstream>

#define BESS_LABEL_SIZE 4
#define BESS_MAJOR_REV 1

namespace {

inline unsigned long get32(std::istringstream &file) {
	unsigned long tmp = file.get() & 0xFF;
	tmp |= (file.get() & 0xFF) << 8;
	tmp |= (file.get() & 0xFF) << 16;
	tmp |= (file.get() & 0xFF) << 24;
	return tmp;
}

inline unsigned short get16(std::istringstream &file) {
	unsigned short tmp = file.get() & 0xFF;
	tmp |= (file.get() & 0xFF) << 8;
	return tmp;
}

inline unsigned char get8(std::istringstream &file) {
	return file.get() & 0xFF;
}

static const char *bessStr = "BESS";
static const char *coreStr = "CORE";
static const char *xoamStr = "XOAM";
static const char *mbcStr  = "MBC ";
static const char *rtcStr  = "RTC ";
static const char *huc3Str = "HUC3";
static const char *sgbStr  = "SGB ";
static const char *endStr  = "END ";

static const char *dmgModelStr = "G";
static const char *cgbModelStr = "C";
static const char *sgbModelStr = "S";

}

namespace gambatte {

// todo: save bess state

int Bess::loadBlock(SaveState &state, std::istringstream &file, int mode) {
	Array<char> const labelbuf(BESS_LABEL_SIZE);
	file.read(labelbuf, 4);
	unsigned long const blockLen = get32(file);
	unsigned long const nextBlockPos = blockLen + file.tellg();

	if (!std::memcmp(labelbuf, coreStr, BESS_LABEL_SIZE)) {
		if (get16(file) != BESS_MAJOR_REV)
			return -1;

		file.ignore(2); // minor rev, ignore for now
		char model = get8(file);
		switch (mode) {
			case GB::LoadFlag::NONE: // DMG_MODE
				if (model != *dmgModelStr)
					return -1;

				break;
			case GB::LoadFlag::CGB_MODE:
				if (model != *cgbModelStr)
					return -1;

				break;
			case GB::LoadFlag::SGB_MODE:
				if (model != *sgbModelStr)
					return -1;

				break;
			default:
				return -1;
		}
		file.ignore(3); // ignore minor model details

		state.cpu.pc = get16(file);
		state.cpu.f = get8(file);
		state.cpu.a = get8(file);
		state.cpu.c = get8(file);
		state.cpu.b = get8(file);
		state.cpu.e = get8(file);
		state.cpu.d = get8(file);
		state.cpu.l = get8(file);
		state.cpu.h = get8(file);
		state.cpu.sp = get16(file);
		state.mem.IME = get8(file);
		((unsigned char *)state.mem.ioamhram.get())[0x1FF] = get8(file);
		unsigned executionState = get8(file);
		switch (executionState) {
			case 0:
				state.mem.halted = 0;
				state.mem.stopped = 0;
				break;
			case 1:
				state.mem.halted = 1;
				state.mem.stopped = 0;
				break;
			case 2:
				state.mem.halted = 1;
				state.mem.stopped = 1;
				break;
			default:
				return -1;
		}
		file.ignore();
		file.read((char *)&state.mem.ioamhram.get()[0x100], 0x80);

		char * ptrs[7] = {
			(char *)state.mem.wram.get(),
			(char *)state.mem.vram.get(),
			(char *)state.mem.sram.get(),
			(char *)state.mem.ioamhram.get(),
			(char *)&state.mem.ioamhram.get()[0x180],
			(char *)state.ppu.bgpData.get(),
			(char *)state.ppu.objpData.get()
		};
		std::size_t lens[7] = {
			state.mem.wram.size(),
			state.mem.vram.size(), 
			state.mem.sram.size(),
			0xA0,
			0x7F,
			state.ppu.bgpData.size(),
			state.ppu.objpData.size()
		};
		unsigned toLoop = (mode == GB::LoadFlag::CGB_MODE && !state.ppu.notCgbDmg) ? 5 : 7;
		for (unsigned i = 0; i < toLoop; i++) {
			unsigned long len = std::min(get32(file), (unsigned long)lens[i]);
			unsigned long bufOffset = get32(file);
			unsigned long temptell = file.tellg();
			file.seekg(bufOffset, std::ios_base::beg);
			file.read(ptrs[i], len);
			file.seekg(temptell, std::ios_base::beg);
		}
	} else if (!std::memcmp(labelbuf, xoamStr, BESS_LABEL_SIZE)) {
		file.read((char *)&state.mem.ioamhram.get()[0xA0], std::min(blockLen, 0x60ul));
	} else if (!std::memcmp(labelbuf, mbcStr, BESS_LABEL_SIZE)) {
		if (blockLen % 3)
			return -1;

		unsigned regs = blockLen / 3;
		while (regs--) {
			unsigned addr = get16(file);
			unsigned data = get8(file);
			// todo: this is making assumptions about the mbc used here, and probably breaks on some mbcs, re-work this
			switch (addr >> 13 & 7) {
				case 0: // 0x0000-0x1FFF
					state.mem.enableRam = data == 0xA;
					break;
				case 1: // 0x2000-0x3FFF
					state.mem.rombank = addr < 0x3000
					                  ? (state.mem.rombank & 0x100) | data
					                  : (data << 8 & 0x100) | (state.mem.rombank & 0xFF);
					break;
				case 2: // 0x4000-0x5FFF
					state.mem.rambank = data;
					break;
				case 3: // 0x6000-0x7FFF
					state.mem.rambankMode = data;
					break;
				case 4: // 0x8000-0x9FFF
					return -1;
				case 5: // 0xA000-0xBFFF
					break; // allowed, but unsupported by this implementation
				case 6: // 0xC000-0xDFFF
				case 7: // 0xE000-0xFFFF
					return -1;
			}
		}
	} else if (!std::memcmp(labelbuf, rtcStr, BESS_LABEL_SIZE)) {
		state.rtc.dataS = get8(file);
		file.ignore(3);
		state.rtc.dataM = get8(file);
		file.ignore(3);
		state.rtc.dataH = get8(file);
		file.ignore(3);
		state.rtc.dataDl = get8(file);
		file.ignore(3);
		state.rtc.dataDh = get8(file);
		file.ignore(3);
		state.rtc.latchS = get8(file);
		file.ignore(3);
		state.rtc.latchM = get8(file);
		file.ignore(3);
		state.rtc.latchH = get8(file);
		file.ignore(3);
		state.rtc.latchDl = get8(file);
		file.ignore(3);
		state.rtc.latchDh = get8(file);
		file.ignore(3);
		state.time.lastTimeSec = get32(file);
	} else if (!std::memcmp(labelbuf, huc3Str, BESS_LABEL_SIZE)) {
		state.time.lastTimeSec = get32(file);
		file.ignore(4);
		unsigned minutes = get16(file);
		unsigned days = get16(file);
		state.huc3.writingTime = state.huc3.haltTime = state.huc3.dataTime = (days << 12) | minutes;
		// alarm stuff is unimplemented
	} else if (!std::memcmp(labelbuf, sgbStr, BESS_LABEL_SIZE)) {
		char * ptrs[7] = {
			(char *)state.mem.sgb.systemTiles.get(),
			(char *)state.mem.sgb.systemTilemap.get(),
			(char *)state.mem.sgb.systemTileColors.get(),
			(char *)state.mem.sgb.colors.get(),
			(char *)state.mem.sgb.systemColors.get(),
			(char *)state.mem.sgb.attributes.get(),
			(char *)state.mem.sgb.systemAttributes.get()
		};
		std::size_t lens[7] = {
			state.mem.sgb.systemTiles.size(),
			state.mem.sgb.systemTilemap.size() * 2,
			state.mem.sgb.systemTileColors.size() * 2,
			state.mem.sgb.colors.size() * 2,
			state.mem.sgb.systemColors.size() * 2,
			state.mem.sgb.attributes.size(),
			state.mem.sgb.systemAttributes.size()
		};
		for (unsigned i = 0; i < 7; i++) {
			unsigned long len = std::min(get32(file), (unsigned long)lens[i]);
			unsigned long bufOffset = get32(file);
			unsigned long temptell = file.tellg();
			file.seekg(bufOffset, std::ios_base::beg);
			file.read(ptrs[i], len);
			file.seekg(temptell, std::ios_base::beg);
		}
		unsigned multistatus = get8(file);
		state.mem.sgb.joypadMask = ((multistatus >> 4) % 5) - 1;
		state.mem.sgb.joypadIndex = multistatus & state.mem.sgb.joypadMask;
	} else if (!std::memcmp(labelbuf, endStr, BESS_LABEL_SIZE)) {
		unsigned long const cc = state.mem.ioamhram.get()[0x144] << 8;
		state.cpu.cycleCounter = cc;
		state.mem.lastOamDmaUpdate = disabled_time;
		state.mem.lastCartBusUpdate = cc;
		state.mem.nextSerialtime = disabled_time;
		state.mem.unhaltTime = cc;
		state.mem.minIntTime = cc;
		state.mem.divLastUpdate = 0;
		state.mem.timaLastUpdate = cc;
		state.mem.tmatime = cc;
		state.mem.hdmaTransfer = 0;
		state.time.lastCycles = cc;
		state.camera.lastCycles = cc;
		state.ppu.lastM0Time = cc;
		state.ppu.enableDisplayM0Time = cc;
		state.ppu.lyc = state.mem.ioamhram.get()[0x145];
		state.ppu.pendingLcdstatIrq = 0;
		state.ppu.oldWy = state.mem.ioamhram.get()[0x14A];
		state.ppu.nextM0Irq = 0;
		state.spu.cycleCounter = cc >> 1;
		return 1;
	}

	file.seekg(nextBlockPos, std::ios_base::beg);
	return 0;
}

bool Bess::loadState(SaveState &state, char const *stateBuf, std::size_t size, int mode) {
	std::istringstream file(std::string(stateBuf, size));
	if (!file)
		return false;

	Array<char> const labelbuf(BESS_LABEL_SIZE);

	// check if this is a valid BESS state
	file.seekg(-4, std::ios_base::end);
	file.read(labelbuf, 4);
	if (std::memcmp(labelbuf, bessStr, BESS_LABEL_SIZE))
		return false;

	file.seekg(-8, std::ios_base::end);
	unsigned long blockOffset = get32(file);
	file.seekg(blockOffset, std::ios_base::beg);

	int status;
	while (file.good()) {
		status = loadBlock(state, file, mode);
		if (status)
			break;
	}
	

	return file.good() && status > 0;
}

bool Bess::loadState(SaveState &state, std::string const &filename, int mode) {
	std::ifstream file(filename.c_str(), std::ios_base::binary);
	if (!file)
		return false;

	std::stringstream fileBuf;
	fileBuf << file.rdbuf();
	std::string const &stateBuf(fileBuf.str());

	return loadState(state, stateBuf.c_str(), stateBuf.size(), mode);
}

}
