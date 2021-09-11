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

#include "initstate.h"
#include "mem_dumps.h"
#include "counterdef.h"
#include "savestate.h"
#include "sound/sound_unit.h"
#include "mem/time.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <iostream>

void gambatte::setInitState(SaveState &state, bool const cgb, bool const sgb, bool const agb, std::size_t const stalledCycles) {
	static unsigned char const cgbObjpDump[0x40] = {
		0x00, 0x00, 0xF2, 0xAB,
		0x61, 0xC2, 0xD9, 0xBA,
		0x88, 0x6E, 0xDD, 0x63,
		0x28, 0x27, 0xFB, 0x9F,
		0x35, 0x42, 0xD6, 0xD4,
		0x50, 0x48, 0x57, 0x5E,
		0x23, 0x3E, 0x3D, 0xCA,
		0x71, 0x21, 0x37, 0xC0,
		0xC6, 0xB3, 0xFB, 0xF9,
		0x08, 0x00, 0x8D, 0x29,
		0xA3, 0x20, 0xDB, 0x87,
		0x62, 0x05, 0x5D, 0xD4,
		0x0E, 0x08, 0xFE, 0xAF,
		0x20, 0x02, 0xD7, 0xFF,
		0x07, 0x6A, 0x55, 0xEC,
		0x83, 0x40, 0x0B, 0x77
	};

	state.cpu.cycleCounter = (8 - stalledCycles) & 0xFFFF;
	state.cpu.pc = 0;
	state.cpu.sp = 0;
	state.cpu.a = 0;
	state.cpu.b = 0;
	state.cpu.c = 0;
	state.cpu.d = 0;
	state.cpu.e = 0;
	state.cpu.f = 0;
	state.cpu.h = 0;
	state.cpu.l = 0;
	state.cpu.opcode = 0x00;
	state.cpu.prefetched = false;
	state.cpu.skip = false;
	state.mem.biosMode = true;

	if (cgb) {
		if (agb)
			setInitialAgbIoamhram(state.mem.ioamhram.ptr);
		else
			setInitialCgbIoamhram(state.mem.ioamhram.ptr);
	} else
		setInitialDmgIoamhram(state.mem.ioamhram.ptr);

	state.mem.ioamhram.ptr[0x104] = 0;
	state.mem.ioamhram.ptr[0x140] = 0;
	state.mem.ioamhram.ptr[0x144] = 0x00;

	// DIV, TIMA, and the PSG frame sequencer are clocked by bits of the
	// cycle counter less divLastUpdate (equivalent to a counter that is
	// reset on DIV write).
	state.mem.divLastUpdate = 0;
	state.mem.timaLastUpdate = 0;
	state.mem.tmatime = disabled_time;
	state.mem.nextSerialtime = disabled_time;
	state.mem.lastOamDmaUpdate = disabled_time;
	state.mem.unhaltTime = disabled_time;
	state.mem.lastCartBusUpdate = 0;
	state.mem.minIntTime = 0;
	state.mem.rombank = 1;
	state.mem.dmaSource = 0;
	state.mem.dmaDestination = 0;
	state.mem.rambank = 0;
	state.mem.oamDmaPos = 0xFE;
	state.mem.haltHdmaState = 0;
	state.mem.IME = false;
	state.mem.halted = false;
	state.mem.enableRam = false;
	state.mem.rambankMode = false;
	state.mem.hdmaTransfer = false;
	state.mem.stopped = false;


	std::memset(state.mem.sgb.systemTilemap.ptr, 0, state.mem.sgb.systemTilemap.size() * 2);
	std::memset(state.mem.sgb.tilemap.ptr, 0, state.mem.sgb.tilemap.size() * 2);
	std::memset(state.mem.sgb.systemTileColors.ptr, 0, state.mem.sgb.systemTileColors.size() * 2);
	std::memset(state.mem.sgb.tileColors.ptr, 0, state.mem.sgb.tileColors.size() * 2);
	std::memset(state.mem.sgb.systemColors.ptr, 0, state.mem.sgb.systemColors.size() * 2);
	std::memset(state.mem.sgb.colors.ptr, 0, state.mem.sgb.colors.size() * 2);
	std::memset(state.mem.sgb.systemAttributes.ptr, 0, state.mem.sgb.systemAttributes.size());	
	std::memset(state.mem.sgb.attributes.ptr, 0, state.mem.sgb.attributes.size());
	std::memset(state.mem.sgb.systemTiles.ptr, 0, state.mem.sgb.systemTiles.size());
	std::memset(state.mem.sgb.tiles.ptr, 0, state.mem.sgb.tiles.size());
	std::memset(state.mem.sgb.packet.ptr, 0, state.mem.sgb.packet.size());
	std::memset(state.mem.sgb.command.ptr, 0, state.mem.sgb.command.size());
	std::memset(state.mem.sgb.frameBuf.ptr, 0, state.mem.sgb.frameBuf.size());
	std::memset(state.mem.sgb.soundControl.ptr, 0, state.mem.sgb.soundControl.size());

	if (sgb) {
		state.mem.sgb.colors.ptr[0] = 0x67BF;

		for (int i = 0; i < 16; i += 4) {
			state.mem.sgb.colors.ptr[i + 1] = 0x265B;
			state.mem.sgb.colors.ptr[i + 2] = 0x10B5;
			state.mem.sgb.colors.ptr[i + 3] = 0x2866;
		}

		setInitialSpcState(state.mem.sgb.spcState.ptr);
	}

	state.mem.sgb.transfer = 0xFF;
	state.mem.sgb.commandIndex = 0;
	state.mem.sgb.joypadIndex = 0;
	state.mem.sgb.joypadMask = 0;
	state.mem.sgb.borderFade = 0;
	state.mem.sgb.pending = 0xFF;
	state.mem.sgb.pendingCount = 0;
	state.mem.sgb.mask = 0;

	for (int i = 0x00; i < 0x40; i += 0x02) {
		state.ppu.bgpData.ptr[i    ] = 0xFF;
		state.ppu.bgpData.ptr[i + 1] = 0x7F;
	}

	std::memcpy(state.ppu.objpData.ptr, cgbObjpDump, sizeof cgbObjpDump);

	if (!cgb) {
		state.ppu.bgpData.ptr[0] = state.mem.ioamhram.get()[0x147];
		state.ppu.objpData.ptr[0] = state.mem.ioamhram.get()[0x148];
		state.ppu.objpData.ptr[1] = state.mem.ioamhram.get()[0x149];
	}

	for (int pos = 0; pos < 80; ++pos)
		state.ppu.oamReaderBuf.ptr[pos] = state.mem.ioamhram.ptr[(pos * 2 & ~3) | (pos & 1)];

	std::fill_n(state.ppu.oamReaderSzbuf.ptr, 40, false);
	std::memset(state.ppu.spAttribList, 0, sizeof state.ppu.spAttribList);
	std::memset(state.ppu.spByte0List, 0, sizeof state.ppu.spByte0List);
	std::memset(state.ppu.spByte1List, 0, sizeof state.ppu.spByte1List);
	state.ppu.videoCycles = 0;
	state.ppu.enableDisplayM0Time = state.cpu.cycleCounter;
	state.ppu.winYPos = 0xFF;
	state.ppu.xpos = 0;
	state.ppu.endx = 0;
	state.ppu.reg0 = 0;
	state.ppu.reg1 = 0;
	state.ppu.tileword = 0;
	state.ppu.ntileword = 0;
	state.ppu.attrib = 0;
	state.ppu.nattrib = 0;
	state.ppu.state = 0;
	state.ppu.nextSprite = 0;
	state.ppu.currentSprite = 0;
	state.ppu.lyc   = state.mem.ioamhram.get()[0x145];
	state.ppu.m0lyc = state.mem.ioamhram.get()[0x145];
	state.ppu.weMaster = false;
	state.ppu.winDrawState = 0;
	state.ppu.wscx = 0;
	state.ppu.lastM0Time = 1234;
	state.ppu.nextM0Irq = 0;
	state.ppu.oldWy = state.mem.ioamhram.get()[0x14A];
	state.ppu.pendingLcdstatIrq = false;
	state.ppu.notCgbDmg = true;

	// spu.cycleCounter >> 12 & 7 represents the frame sequencer position.
	state.spu.cycleCounter = state.cpu.cycleCounter >> 1;
	state.spu.lastUpdate = 0;

	state.spu.ch1.sweep.counter = SoundUnit::counter_disabled;
	state.spu.ch1.sweep.shadow = 0;
	state.spu.ch1.sweep.nr0 = 0;
	state.spu.ch1.sweep.neg = false;
	state.spu.ch1.duty.nextPosUpdate = SoundUnit::counter_disabled;
	state.spu.ch1.duty.pos = 0;
	state.spu.ch1.duty.high = false;
	state.spu.ch1.duty.nr3 = 0;
	state.spu.ch1.env.counter = SoundUnit::counter_disabled;
	state.spu.ch1.env.volume = 0;
	state.spu.ch1.lcounter.counter = SoundUnit::counter_disabled;
	state.spu.ch1.lcounter.lengthCounter = 0;
	state.spu.ch1.nr4 = 0;
	state.spu.ch1.master = false;

	state.spu.ch2.duty.nextPosUpdate = SoundUnit::counter_disabled;
	state.spu.ch2.duty.nr3 = 0;
	state.spu.ch2.duty.pos = 0;
	state.spu.ch2.duty.high = false;
	state.spu.ch2.env.counter = SoundUnit::counter_disabled;
	state.spu.ch2.env.volume = 0;
	state.spu.ch2.lcounter.counter = SoundUnit::counter_disabled;
	state.spu.ch2.lcounter.lengthCounter = 0;
	state.spu.ch2.nr4 = 0;
	state.spu.ch2.master = false;

	std::memcpy(state.spu.ch3.waveRam.ptr, state.mem.ioamhram.get() + 0x130, 0x10);
	state.spu.ch3.lcounter.counter = SoundUnit::counter_disabled;
	state.spu.ch3.lcounter.lengthCounter = 0x100;
	state.spu.ch3.waveCounter = SoundUnit::counter_disabled;
	state.spu.ch3.lastReadTime = SoundUnit::counter_disabled;
	state.spu.ch3.nr3 = 0;
	state.spu.ch3.nr4 = 0;
	state.spu.ch3.wavePos = 0;
	state.spu.ch3.sampleBuf = 0;
	state.spu.ch3.master = false;

	state.spu.ch4.lfsr.counter = state.spu.cycleCounter + 4;
	state.spu.ch4.lfsr.reg = 0xFF;
	state.spu.ch4.env.counter = SoundUnit::counter_disabled;
	state.spu.ch4.env.volume = 0;
	state.spu.ch4.lcounter.counter = SoundUnit::counter_disabled;
	state.spu.ch4.lcounter.lengthCounter = 0;
	state.spu.ch4.nr4 = 0;
	state.spu.ch4.master = false;

	state.huc3.haltTime = state.time.seconds;
	state.huc3.dataTime = 0;
	state.huc3.writingTime = 0;
	state.huc3.halted = false;
	state.huc3.shift = 0;
	state.huc3.ramValue = 1;
	state.huc3.modeflag = 2; // huc3_none

	std::memset(   state.camera.matrix.ptr, 0,    state.camera.matrix.size());
	std::memset(state.camera.oldMatrix.ptr, 0, state.camera.oldMatrix.size());

	state.camera.trigger = 0;
	state.camera.n = false;
	state.camera.vh = 0;
	state.camera.exposure = 0;
	state.camera.edgeAlpha = 0.50 * 4;
	state.camera.blank = false;
	state.camera.invert = false;
	state.camera.oldTrigger = 0;
	state.camera.oldN = false;
	state.camera.oldVh = 0;
	state.camera.oldExposure = 0;
	state.camera.oldEdgeAlpha = 0.50 * 4;
	state.camera.oldBlank = false;
	state.camera.oldInvert = false;
	state.camera.lastCycles = state.cpu.cycleCounter;
	state.camera.cameraCyclesLeft = 0;
	state.camera.cancelled = false;
}

void gambatte::setInitStateCart(SaveState &state, const bool cgb, const bool agb) {
	setInitialVram(state.mem.vram.ptr, cgb);

	if (cgb) {
		if (agb)
			setInitialAgbWram(state.mem.wram.ptr);
		else
			setInitialCgbWram(state.mem.wram.ptr);
	} else
		setInitialDmgWram(state.mem.wram.ptr);

	std::memset(state.mem.sram.ptr, 0xFF, state.mem.sram.size());

	state.time.seconds = 0;
	state.time.lastTimeSec = Time::now().tv_sec;
	state.time.lastTimeUsec = Time::now().tv_usec;
	state.time.lastCycles = state.cpu.cycleCounter;

	state.rtc.dataDh = 0;
	state.rtc.dataDl = 0;
	state.rtc.dataH = 0;
	state.rtc.dataM = 0;
	state.rtc.dataS = 0;
	state.rtc.dataC = 0;
	state.rtc.latchDh = 0;
	state.rtc.latchDl = 0;
	state.rtc.latchH = 0;
	state.rtc.latchM = 0;
	state.rtc.latchS = 0;
}

void gambatte::setPostBiosState(SaveState &state, bool const cgb, bool const agb, bool const notCgbDmg) {
	state.cpu.cycleCounter = cgb ? 0x102A0 + (agb * 0x4) : 0x102A0 + 0x8D2C;
	state.cpu.pc = 0x100;
	state.cpu.sp = 0xFFFE;
	state.cpu.a = cgb * 0x10 | 0x01;
	state.cpu.b = cgb & agb;
	state.cpu.c = 0x13;
	state.cpu.d = 0x00;
	state.cpu.e = 0xD8;
	state.cpu.f = 0xB0;
	state.cpu.h = 0x01;
	state.cpu.l = 0x4D;
	state.mem.biosMode = false;

	// some games will rely on bios initalization of VRAM
	setInitialVram(state.mem.vram.ptr, cgb);

	state.mem.ioamhram.ptr[0x111] = 0xBF;
	state.mem.ioamhram.ptr[0x112] = 0xF3;
	state.mem.ioamhram.ptr[0x124] = 0x77;
	state.mem.ioamhram.ptr[0x125] = 0xF3;
	state.mem.ioamhram.ptr[0x126] = 0xF1;
	state.mem.ioamhram.ptr[0x140] = 0x91;

	state.mem.ioamhram.ptr[0x1FC] -= agb;

	state.mem.divLastUpdate = -0x1C00;

	state.ppu.videoCycles = cgb ? 144*456ul + 164 + (agb * 4) : 153*456ul + 396;
	state.ppu.enableDisplayM0Time = state.cpu.cycleCounter;
	state.ppu.notCgbDmg = cgb && notCgbDmg;

	state.spu.cycleCounter = (cgb ? 0x1E00 : 0x2400) | (state.cpu.cycleCounter >> 1 & 0x1FF);

	if (cgb) {
		state.spu.ch1.duty.nextPosUpdate = (state.spu.cycleCounter & ~1ul) + (37 - agb) * 2;
		state.spu.ch1.duty.pos = 6;
		state.spu.ch1.duty.high = true;
	} else {
		state.spu.ch1.duty.nextPosUpdate = (state.spu.cycleCounter & ~1ul) + 69 * 2;
		state.spu.ch1.duty.pos = 3;
		state.spu.ch1.duty.high = false;
	}
	state.spu.ch1.duty.nr3 = 0xC1;
	state.spu.ch1.lcounter.lengthCounter = 0x40;
	state.spu.ch1.nr4 = 0x07;
	state.spu.ch1.master = true;

	state.spu.ch2.lcounter.lengthCounter = 0x40;

	state.spu.ch4.lfsr.counter = state.spu.cycleCounter + 4;
	state.spu.ch4.lcounter.lengthCounter = 0x40;
}
