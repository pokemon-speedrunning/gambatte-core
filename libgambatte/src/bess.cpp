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

#include "statesaver.h"
#include "savestate.h"
#include "gambatte.h"
#include "array.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>
#include <cstring>

#define BESS_LABEL_SIZE 4
#define BESS_MAJOR_REV 1

namespace gambatte {

unsigned long get32(std::istringstream &file) {
	unsigned long tmp = file.get() & 0xFF;
	tmp =   tmp << 8 | (file.get() & 0xFF);
	tmp =   tmp << 8 | (file.get() & 0xFF);
	return  tmp << 8 | (file.get() & 0xFF);
}

unsigned long get24(std::istringstream &file) {
	unsigned long tmp = file.get() & 0xFF;
	tmp =   tmp << 8 | (file.get() & 0xFF);
	return  tmp << 8 | (file.get() & 0xFF);
}

unsigned short get16(std::istringstream &file) {
	unsigned short tmp = file.get() & 0xFF;
	return  tmp << 8 | (file.get() & 0xFF);
}

class Bess;

static const *char bessStr = "BESS";

static const *char coreStr = "CORE";
static const *char xoamStr = "XOAM";
static const *char mbcStr  = "MBC ";
static const *char rtcStr  = "RTC ";
static const *char huc3Str = "HUC3";
static const *char sgbStr  = "SGB ";
static const *char endStr  = "END ";

// todo: save bess state

int Bess::loadBlock(SaveState &state, std::istringstream &file, int mode) {
	Array<char> const labelbuf(BESS_LABEL_SIZE);
	Array<char> const lenbuf(BESS_LABEL_SIZE);

	file.read(labelbuf, 4);
	file.read(lenBuf, 4);
	unsigned long len = lenBuf[0] | (lenBuf[1] << 8) | (lenBuf[2] << 16) | (lenBuf[3] << 24); 
	unsigned long blockPos = file.tellg();

	if (!std::memcmp(labelbuf, coreStr, BESS_LABEL_SIZE)) {
		if (get16(file) != BESS_MAJOR_REV)
			return -1;

		file.ignore(2); // minor rev, ignore for now
		char model = file.get() & 0xFF;
		switch (mode) {
			case GB::LoadFlag::NONE: // DMG_MODE
				if (model != "G")
					return -1;

				break;
			case GB::LoadFlag::CGB_MODE:
				if (model != "C")
					return -1;

				break;
			case GB::LoadFlag::SGB_MODE:
				if (model != "S")
					return -1;

				break;
			default:
				return -1;
		}
		file.ignore(3); // ignore minor model details
		state.cpu.pc = get16(file);
		state.cpu.f = file.get() & 0xFF;
		state.cpu.a = file.get() & 0xFF;
		state.cpu.c = file.get() & 0xFF;
		state.cpu.b = file.get() & 0xFF;
		state.cpu.e = file.get() & 0xFF;
		state.cpu.d = file.get() & 0xFF;
		state.cpu.l = file.get() & 0xFF;
		state.cpu.h = file.get() & 0xFF;
		state.cpu.sp = get16(file);
		state.mem.IME = file.get() & 0xFF;
		state.mem.ioamhram.ptr[0x1FF] = file.get() & 0xFF;
		unsigned state = file.get() & 0xFF;
		switch (state) {
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
		file.read(&state.mem.ioamhram.ptr[0x100], 0x80);

		unsigned char * ptrs[7] = { state.mem.wram.ptr, state.mem.vram.ptr, state.mem.sram.ptr, state.mem.ioamhram.ptr, &state.mem.ioamhram.ptr[0x180], state.ppu.bgpData.ptr, state.ppu.objpData.ptr } 
		unsigned long lens[7] = { state.mem.wram.size(), state.mem.vram.size(),  state.mem.sram.size(), 0xA0, 0x7F, state.ppu.bgpData.size(), state.ppu.objpData.size() }
		for (unsigned i = 0; i < 7; i++) {
			std::memset(ptrs[i], 0, lens[i]);
			unsigned long len = std::min(get32(file), lens[i]);
			unsigned bufOffset = get32(file);
			unsigned temptell = file.tellg();
			file.seekg(bufOffset, std::ios_base::beg);
			file.read(ptrs[i], len);
			file.seekg(temptell, std::ios_base::beg);
		}
	} else if (!std::memcmp(labelbuf, xoamStr, BESS_LABEL_SIZE)) {
		
	} else if (!std::memcmp(labelbuf, mbcStr, BESS_LABEL_SIZE)) {
		
	} else if (!std::memcmp(labelbuf, rtcStr, BESS_LABEL_SIZE)) {
		
	} else if (!std::memcmp(labelbuf, huc3Str, BESS_LABEL_SIZE)) {
		
	} else if (!std::memcmp(labelbuf, sgbStr, BESS_LABEL_SIZE)) {
		
	} else if (!std::memcmp(labelbuf, endStr, BESS_LABEL_SIZE)) {
		return 1;
	} else {
		file.seekg(blockPos + len, std::ios_base::beg);
		return 0;
	}
}

bool Bess::loadState(SaveState &state, char const *stateBuf, std::size_t size, int mode) {
	std::istringstream file(std::string(stateBuf, size));
	if (!file)
		return false;

	Array<char> const labelbuf(BESS_LABEL_SIZE);

	// check if this is a valid BESS state
	file.seekg(-4, ios_base::end);
	file.read(labelbuf, 4);
	if (std::memcmp(labelbuf, bessStr, BESS_LABEL_SIZE))
		return false;

	file.seekg(-8, ios_base::end);
	unsigned blockOffset = get32(file); 
	file.seekg(blockOffset, std::ios_base::beg);

	int status;
	while (file.good()) {
		if (status = loadBlock(state, file, mode))
			break;
	}

	state.cpu.cycleCounter &= 0x7FFFFFFF;
	state.spu.cycleCounter &= 0x7FFFFFFF;

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
