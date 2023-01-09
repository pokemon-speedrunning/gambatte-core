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

void gambatte::setInitState(SaveState &state, bool const cgb, bool const sgb, bool const agb, std::size_t const stalledCycles, char const *romTitlePtr) {
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
	state.mem.ramflag = 0;
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
		unsigned short palettes[4];
		char romTitle[0x10];
		std::memcpy(romTitle, romTitlePtr, sizeof romTitle);
		if (!std::strcmp(romTitle, "SUPERMARIOLAND3")) {
			palettes[0] = 0x637B;
			palettes[1] = 0x3AD9;
			palettes[2] = 0x0956;
			palettes[3] = 0x0000;
		} else if (!std::strcmp(romTitle, "KIRBY'S PINBALL")) {
			palettes[0] = 0x7F1F;
			palettes[1] = 0x2A7D;
			palettes[2] = 0x30F3;
			palettes[3] = 0x4CE7;
		} else if (!std::strcmp(romTitle, "YOSSY NO COOKIE") || !std::strcmp(romTitle, "YOSHI'S COOKIE")) {
			palettes[0] = 0x57FF;
			palettes[1] = 0x2618;
			palettes[2] = 0x001F;
			palettes[3] = 0x006A;
		} else if (!std::strcmp(romTitle, "ZELDA")) {
			palettes[0] = 0x5B7F;
			palettes[1] = 0x3F0F;
			palettes[2] = 0x222D;
			palettes[3] = 0x10EB;
		} else if (!std::strcmp(romTitle, "SUPER MARIOLAND")) {
			palettes[0] = 0x7FBB;
			palettes[1] = 0x2A3C;
			palettes[2] = 0x0015;
			palettes[3] = 0x0900;
		} else if (!std::strcmp(romTitle, "SOLARSTRIKER")) {
			palettes[0] = 0x2800;
			palettes[1] = 0x7680;
			palettes[2] = 0x01EF;
			palettes[3] = 0x2FFF;
		} else if (!std::strcmp(romTitle, "KAERUNOTAMENI")) {
			palettes[0] = 0x533E;
			palettes[1] = 0x2638;
			palettes[2] = 0x01E5;
			palettes[3] = 0x0000;
		} else if (!std::strcmp(romTitle, "HOSHINOKA-BI") || !std::strcmp(romTitle, "KIRBY DREAM LAND")) {
			palettes[0] = 0x7F1F;
			palettes[1] = 0x463D;
			palettes[2] = 0x74CF;
			palettes[3] = 0x4CA5;
		} else if (!std::strcmp(romTitle, "YOSSY NO TAMAGO") || !std::strcmp(romTitle, "MARIO & YOSHI")) {
			palettes[0] = 0x53FF;
			palettes[1] = 0x03E0;
			palettes[2] = 0x00DF;
			palettes[3] = 0x2800;
		} else if (!std::strcmp(romTitle, "KID ICARUS")) {
			palettes[0] = 0x7FFA;
			palettes[1] = 0x2A5F;
			palettes[2] = 0x0014;
			palettes[3] = 0x0003;
		} else if (!std::strcmp(romTitle, "BASEBALL")) {
			palettes[0] = 0x1EED;
			palettes[1] = 0x215C;
			palettes[2] = 0x42FC;
			palettes[3] = 0x0060;
		} else if (!std::strcmp(romTitle, "TETRIS")) {
			palettes[0] = 0x4F5F;
			palettes[1] = 0x630E;
			palettes[2] = 0x159F;
			palettes[3] = 0x3126;
		} else if (!std::strcmp(romTitle, "DR.MARIO")) {
			palettes[0] = 0x637B;
			palettes[1] = 0x121C;
			palettes[2] = 0x0140;
			palettes[3] = 0x0840;
		} else if (!std::strcmp(romTitle, "YAKUMAN")) {
			palettes[0] = 0x66BC;
			palettes[1] = 0x3FFF;
			palettes[2] = 0x7EE0;
			palettes[3] = 0x2C84;
		} else if (!std::strcmp(romTitle, "MARIOLAND2")) {
			palettes[0] = 0x5FFE;
			palettes[1] = 0x3EBC;
			palettes[2] = 0x0321;
			palettes[3] = 0x0000;
		} else if (!std::strcmp(romTitle, "GBWARS")) {
			palettes[0] = 0x63FF;
			palettes[1] = 0x36DC;
			palettes[2] = 0x11F6;
			palettes[3] = 0x392A;
		} else if (!std::strcmp(romTitle, "ALLEY WAY")) {
			palettes[0] = 0x65EF;
			palettes[1] = 0x7DBF;
			palettes[2] = 0x035F;
			palettes[3] = 0x2108;
		} else if (!std::strcmp(romTitle, "TENNIS")) {
			palettes[0] = 0x2B6C;
			palettes[1] = 0x7FFF;
			palettes[2] = 0x1CD9;
			palettes[3] = 0x0007;
		} else if (!std::strcmp(romTitle, "GOLF")) {
			palettes[0] = 0x53FC;
			palettes[1] = 0x1F2F;
			palettes[2] = 0x0E29;
			palettes[3] = 0x0061;
		} else if (!std::strcmp(romTitle, "QIX")) {
			palettes[0] = 0x36BE;
			palettes[1] = 0x7EAF;
			palettes[2] = 0x681A;
			palettes[3] = 0x3C00;
		} else if (!std::strcmp(romTitle, "X")) {
			palettes[0] = 0x5FFF;
			palettes[1] = 0x6732;
			palettes[2] = 0x3DA9;
			palettes[3] = 0x2481;
		} else if (!std::strcmp(romTitle, "F1RACE")) {
			palettes[0] = 0x6B57;
			palettes[1] = 0x6E1B;
			palettes[2] = 0x5010;
			palettes[3] = 0x0007;
		} else if (!std::strcmp(romTitle, "METROID2")) {
			palettes[0] = 0x0F96;
			palettes[1] = 0x2C97;
			palettes[2] = 0x0045;
			palettes[3] = 0x3200;
		} else {
			palettes[0] = 0x67BF;
			palettes[1] = 0x265B;
			palettes[2] = 0x10B5;
			palettes[3] = 0x2866;
		}

		state.mem.sgb.colors.ptr[0] = palettes[0];

		for (int i = 0; i < 16; i += 4) {
			state.mem.sgb.colors.ptr[i + 1] = palettes[1];
			state.mem.sgb.colors.ptr[i + 2] = palettes[2];
			state.mem.sgb.colors.ptr[i + 3] = palettes[3];
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
	state.mem.sgb.samplesAccumulated = 0;

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
	state.ppu.spPriority = (state.mem.ioamhram.get()[0x16C] & 1) | 2;
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

	state.ir.irTrigger = true;
	state.ir.thisGbIrSignal = false;

	state.ir.remote.isActive = false;
	state.ir.remote.lastUpdate = 0;
	state.ir.remote.cyclesElapsed = 0;
	state.ir.remote.command = 0;

	state.huc3.ioIndex = 0;
	state.huc3.transferValue = 0;
	state.huc3.ramflag = 0;
	state.huc3.currentSample = 0;
	state.huc3.toneLastUpdate = 0;
	state.huc3.nextPhaseChangeTime = 0;
	state.huc3.remainingToneSamples = 0;
	state.huc3.committing = false;
	state.huc3.highIoReadOnly = true; // ???

	std::memset(state.camera.matrix.ptr, 0, state.camera.matrix.size());

	state.camera.trigger = 0;
	state.camera.n = false;
	state.camera.vh = 0;
	state.camera.exposure = 0;
	state.camera.edgeAlpha = 2;
	state.camera.blank = false;
	state.camera.invert = false;
	state.camera.lastCycles = state.cpu.cycleCounter;
	state.camera.cameraCyclesLeft = 0;
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

	std::memset(state.huc3.io.ptr, 0, state.huc3.io.size());

	state.huc3.rtcCycles = 0;

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

	state.mem.ioamhram.ptr[0x16C] |= !(cgb && notCgbDmg);
	state.mem.ioamhram.ptr[0x1FC] -= agb;

	state.mem.divLastUpdate = -0x1C00;

	if (!notCgbDmg) {
		state.ppu.bgpData.ptr[0] = state.mem.ioamhram.get()[0x147];
		state.ppu.objpData.ptr[0] = state.mem.ioamhram.get()[0x148];
		state.ppu.objpData.ptr[1] = state.mem.ioamhram.get()[0x149];
	}

	state.ppu.videoCycles = cgb ? 144*456ul + 164 + (agb * 4) : 153*456ul + 396;
	state.ppu.enableDisplayM0Time = state.cpu.cycleCounter;
	state.ppu.notCgbDmg = cgb && notCgbDmg;
	state.ppu.spPriority = 2 | !(cgb && notCgbDmg);

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
