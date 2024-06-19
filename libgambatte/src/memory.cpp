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

#include "memory.h"
#include "gambatte.h"
#include "inputgetter.h"
#include "savestate.h"
#include "sound.h"
#include "video.h"
#include "file/crc32.h"

#include <algorithm>
#include <cstring>

using namespace gambatte;

namespace {

int const oam_size = 4 * lcd_num_oam_entries;

void decCycles(unsigned long &counter, unsigned long dec) {
	if (counter != disabled_time)
		counter -= dec;
}

int serialCntFrom(unsigned long cyclesUntilDone, bool cgbFast) {
	return cgbFast ? (cyclesUntilDone + 0xF) >> 4 : (cyclesUntilDone + 0x1FF) >> 9;
}

} // unnamed namespace.

Memory::Memory(Interrupter const &interrupter)
: getInput_(0)
, divLastUpdate_(0)
, lastOamDmaUpdate_(disabled_time)
, lastCartBusUpdate_(0)
, cartBusPullUpTime_(8)
, lcd_(ioamhram_, 0, VideoInterruptRequester(intreq_))
, interrupter_(interrupter)
, dmaSource_(0)
, dmaDestination_(0)
, oamDmaPos_(-2u & 0xFF)
, oamDmaStartPos_(0)
, serialCnt_(0)
, cartBus_(0xFF)
, blanklcd_(false)
, haltHdmaState_(hdma_low)
, readCallback_(0)
, writeCallback_(0)
, execCallback_(0)
, cdCallback_(0)
, linkCallback_(0)
, linked_(false)
, linkClockTrigger_(false)
{
	intreq_.setEventTime<intevent_blit>(1l * lcd_vres * lcd_cycles_per_line);
	intreq_.setEventTime<intevent_end>(0);
}

void Memory::setStatePtrs(SaveState &state) {
	state.mem.ioamhram.set(ioamhram_, sizeof ioamhram_);

	cart_.setStatePtrs(state);
	sgb_.setStatePtrs(state, isSgb());
	lcd_.setStatePtrs(state);
	psg_.setStatePtrs(state);
}

unsigned long Memory::saveState(SaveState &state, unsigned long cc) {
	cc = resetCounters(cc);
	ioamhram_[0x104] = 0;
	nontrivial_ff_read(0x05, cc);
	nontrivial_ff_read(0x0F, cc);
	nontrivial_ff_read(0x26, cc);

	state.mem.divLastUpdate = divLastUpdate_;
	state.mem.nextSerialtime = intreq_.eventTime(intevent_serial);
	state.mem.unhaltTime = intreq_.eventTime(intevent_unhalt);
	state.mem.lastOamDmaUpdate = oamDmaStartPos_
		? lastOamDmaUpdate_ + ((oamDmaStartPos_ - oamDmaPos_) & 0xFF) * 4
		: lastOamDmaUpdate_;
	state.mem.dmaSource = dmaSource_;
	state.mem.dmaDestination = dmaDestination_;
	state.mem.oamDmaPos = oamDmaPos_;
	state.mem.haltHdmaState = haltHdmaState_;
	state.mem.biosMode = biosMode_;
	state.mem.stopped = stopped_;
	state.mem.lastCartBusUpdate = lastCartBusUpdate_;
	state.mem.totalSamplesEmittedHigh = totalSamplesEmitted_ >> 32;
	state.mem.totalSamplesEmittedLow = totalSamplesEmitted_ & 0xFFFFFFFF;

	intreq_.saveState(state);
	cart_.saveState(state, cc);
	sgb_.saveState(state);
	tima_.saveState(state);
	lcd_.saveState(state);
	psg_.saveState(state);

	return cc;
}

void Memory::loadState(SaveState const &state) {
	biosMode_ = state.mem.biosMode;
	stopped_ = state.mem.stopped;
	lastCartBusUpdate_ = state.mem.lastCartBusUpdate;
	totalSamplesEmitted_ = (unsigned long long)state.mem.totalSamplesEmittedHigh << 32;
	totalSamplesEmitted_ |= state.mem.totalSamplesEmittedLow;

	psg_.loadState(state);
	lcd_.loadState(state, state.mem.oamDmaPos < oam_size ? cart_.rdisabledRam() : ioamhram_);
	tima_.loadState(state, TimaInterruptRequester(intreq_));
	sgb_.loadState(state);
	cart_.loadState(state);
	intreq_.loadState(state);

	intreq_.setEventTime<intevent_serial>(state.mem.nextSerialtime > state.cpu.cycleCounter
		? state.mem.nextSerialtime
		: state.cpu.cycleCounter);
	intreq_.setEventTime<intevent_unhalt>(state.mem.unhaltTime);
	lastOamDmaUpdate_ = state.mem.lastOamDmaUpdate;
	dmaSource_ = state.mem.dmaSource;
	dmaDestination_ = state.mem.dmaDestination;
	oamDmaPos_ = state.mem.oamDmaPos;
	oamDmaStartPos_ = 0;
	haltHdmaState_ = static_cast<HdmaState>(std::min(1u * state.mem.haltHdmaState, 1u * hdma_requested));
	serialCnt_ = intreq_.eventTime(intevent_serial) != disabled_time
		? serialCntFrom(intreq_.eventTime(intevent_serial) - state.cpu.cycleCounter,
			ioamhram_[0x102] & (isCgb() & !isCgbDmg()) * 2)
		: 8;

	cart_.setVrambank(ioamhram_[0x14F] & (isCgb() & !isCgbDmg()));
	cart_.setOamDmaSrc(oam_dma_src_off);
	cart_.setWrambank((isCgb() && !isCgbDmg()) && (ioamhram_[0x170] & 0x07) ? ioamhram_[0x170] & 0x07 : 1);

	if (lastOamDmaUpdate_ != disabled_time) {
		if (lastOamDmaUpdate_ > state.cpu.cycleCounter) {
			oamDmaStartPos_ = (oamDmaPos_ + (lastOamDmaUpdate_ - state.cpu.cycleCounter) / 4) & 0xFF;
			lastOamDmaUpdate_ = state.cpu.cycleCounter;
		}
		oamDmaInitSetup();

		unsigned oamEventPos = oamDmaPos_ < oam_size ? oam_size : oamDmaStartPos_;
		intreq_.setEventTime<intevent_oam>(
			lastOamDmaUpdate_ + ((oamEventPos - oamDmaPos_) & 0xFF) * 4);
	}

	intreq_.setEventTime<intevent_blit>(ioamhram_[0x140] & lcdc_en
		? lcd_.nextMode1IrqTime()
		: state.cpu.cycleCounter);
	blanklcd_ = false;

	if (!isCgb() || isCgbDmg())
		std::fill_n(cart_.vramdata() + vrambank_size(), vrambank_size(), 0);
}

void Memory::setEndtime(unsigned long cc, unsigned long inc) {
	if (intreq_.eventTime(intevent_blit) <= cc) {
		intreq_.setEventTime<intevent_blit>(intreq_.eventTime(intevent_blit)
			+ (lcd_cycles_per_frame << isDoubleSpeed()));
	}

	intreq_.setEventTime<intevent_end>(cc + (inc << isDoubleSpeed()));
}

void Memory::updateSerial(unsigned long const cc) {
	if (!linked_) {
		if (intreq_.eventTime(intevent_serial) != disabled_time) {
			if (intreq_.eventTime(intevent_serial) <= cc) {
				ioamhram_[0x101] = (((ioamhram_[0x101] + 1) << serialCnt_) - 1) & 0xFF;
				ioamhram_[0x102] &= 0x7F;
				intreq_.flagIrq(8, intreq_.eventTime(intevent_serial));
				intreq_.setEventTime<intevent_serial>(disabled_time);
			} else {
				int const targetCnt = serialCntFrom(intreq_.eventTime(intevent_serial) - cc,
					ioamhram_[0x102] & (isCgb() & !isCgbDmg()) * 2);
				ioamhram_[0x101] = (((ioamhram_[0x101] + 1) << (serialCnt_ - targetCnt)) - 1) & 0xFF;
				serialCnt_ = targetCnt;
			}
		}
	} else {
		if (intreq_.eventTime(intevent_serial) != disabled_time) {
			if (intreq_.eventTime(intevent_serial) <= cc) {
				linkClockTrigger_ = true;
				intreq_.setEventTime<intevent_serial>(disabled_time);
				if (linkCallback_)
					linkCallback_();
			}
		}
	}
}

void Memory::updateTimaIrq(unsigned long cc) {
	while (intreq_.eventTime(intevent_tima) <= cc)
		tima_.doIrqEvent(TimaInterruptRequester(intreq_));
}

void Memory::updateIrqs(unsigned long cc) {
	updateSerial(cc);
	updateTimaIrq(cc);
	lcd_.update(cc);
}

unsigned long Memory::event(unsigned long cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	switch (intreq_.minEventId()) {
	case intevent_unhalt:
		if ((lcd_.hdmaIsEnabled() && lcd_.isHdmaPeriod(cc) && haltHdmaState_ == hdma_low)
				|| haltHdmaState_ == hdma_requested) {
			flagHdmaReq(intreq_);
		}

		intreq_.unhalt();
		intreq_.setEventTime<intevent_unhalt>(disabled_time);
		break;
	case intevent_end:
		intreq_.setEventTime<intevent_end>(disabled_time - 1);

		while (cc >= intreq_.minEventTime()
				&& intreq_.eventTime(intevent_end) != disabled_time) {
			cc = event(cc);
		}

		intreq_.setEventTime<intevent_end>(disabled_time);

		break;
	case intevent_blit:
		{
			bool const lcden = ioamhram_[0x140] & lcdc_en;
			unsigned long blitTime = intreq_.eventTime(intevent_blit);

			if (lcden | blanklcd_) {
				lcd_.updateScreen(blanklcd_, cc, 0);
				if (isSgb())
					sgb_.updateScreen(blanklcd_);
				lcd_.updateScreen(blanklcd_, cc, 1);

				intreq_.setEventTime<intevent_blit>(disabled_time);
				intreq_.setEventTime<intevent_end>(disabled_time);

				while (cc >= intreq_.minEventTime())
					cc = event(cc);
			} else
				blitTime += lcd_cycles_per_frame << isDoubleSpeed();

			blanklcd_ = lcden ^ 1;
			intreq_.setEventTime<intevent_blit>(blitTime);
		}
		break;
	case intevent_serial:
		updateSerial(cc);
		break;
	case intevent_oam:
		if (lastOamDmaUpdate_ != disabled_time) {
			unsigned const oamEventPos = oamDmaPos_ < oam_size ? oam_size : oamDmaStartPos_;
			intreq_.setEventTime<intevent_oam>(
				lastOamDmaUpdate_ + ((oamEventPos - oamDmaPos_) & 0xFF) * 4);
		} else
			intreq_.setEventTime<intevent_oam>(disabled_time);

		break;
	case intevent_dma:
		interrupter_.prefetch(cc, *this);
		cc = dma(cc);
		if (haltHdmaState_ == hdma_requested) {
			haltHdmaState_ = hdma_low;
			intreq_.setMinIntTime(cc);
			cc -= 4;
		}
		break;
	case intevent_tima:
		tima_.doIrqEvent(TimaInterruptRequester(intreq_));
		break;
	case intevent_video:
		lcd_.update(cc);
		break;
	case intevent_interrupts:
		// GSR NOTE: "make stop really stop" (commit 9a79253, since r649)
		if (stopped_) {
			intreq_.setEventTime<intevent_interrupts>(disabled_time);
			break;
		}

		if (halted()) {
			cc += 4 * (isCgb() || cc - intreq_.eventTime(intevent_interrupts) < 2);
			if (cc > lastOamDmaUpdate_)
				updateOamDma(cc);
			if ((lcd_.hdmaIsEnabled() && lcd_.isHdmaPeriod(cc) && haltHdmaState_ == hdma_low)
					|| haltHdmaState_ == hdma_requested) {
				flagHdmaReq(intreq_);
			}

			intreq_.unhalt();
			intreq_.setEventTime<intevent_unhalt>(disabled_time);
		}
		if (cc >= intreq_.eventTime(intevent_video))
			lcd_.update(cc);
		if (cc >= intreq_.eventTime(intevent_dma))
			break;

		if (ime()) {
			di();
			cc = interrupter_.interrupt(cc, *this);
		}

		break;
	}

	return cc;
}

unsigned long Memory::dma(unsigned long cc) {
	bool const doubleSpeed = isDoubleSpeed();
	unsigned dmaSrc = dmaSource_;
	unsigned dmaDest = dmaDestination_;
	unsigned dmaLength = ((ioamhram_[0x155] & 0x7F) + 1) * 0x10;
	unsigned length = hdmaReqFlagged(intreq_) ? 0x10 : dmaLength;

	if (1ul * dmaDest + length >= 0x10000) {
		length = 0x10000 - dmaDest;
		ioamhram_[0x155] |= 0x80;
	}

	dmaLength -= length;

	if (!(ioamhram_[0x140] & lcdc_en) && gdmaReqFlagged(intreq_)) // FIXME: This is wrong for HDMA, but is it right for GDMA?
		dmaLength = 0;

	unsigned long lOamDmaUpdate = lastOamDmaUpdate_;
	lastOamDmaUpdate_ = disabled_time;

	while (length--) {
		unsigned const src = dmaSrc++ & 0xFFFF;
		unsigned const data = (src & -vrambank_size()) == mm_vram_begin || src >= mm_wram_mirror_begin
			? cartBus(cc + 4) // cart bus seems to pull up sooner here? might be prefetch related
			: read<false, false, false, false>(src, cc);

		cc += 2 + 2 * doubleSpeed;

		if (cc - 3 > lOamDmaUpdate && !halted()) {
			oamDmaPos_ = (oamDmaPos_ + 1) & 0xFF;
			lOamDmaUpdate += 4;
			if (oamDmaPos_ == oamDmaStartPos_)
				startOamDma(lOamDmaUpdate);

			if (oamDmaPos_ < oam_size) {
				unsigned const p = src & 0xFF;
				if (p < oam_size)
					ioamhram_[p] = data;
				else if (!agbFlag_)
					ioamhram_[p & 0xE7] = data;
			} else if (oamDmaPos_ == oam_size) {
				endOamDma(lOamDmaUpdate);
				if (oamDmaStartPos_ == 0)
					lOamDmaUpdate = disabled_time;
			}
		}

		nontrivial_write(mm_vram_begin | dmaDest++ % vrambank_size(), data, cc);
	}

	lastOamDmaUpdate_ = lOamDmaUpdate;
	ackDmaReq(intreq_);
	cc += 4;

	dmaSource_ = dmaSrc;
	dmaDestination_ = dmaDest;
	ioamhram_[0x155] = halted()
		? ioamhram_[0x155] | 0x80
		: ((dmaLength / 0x10 - 1) & 0xFF) | (ioamhram_[0x155] & 0x80);

	if ((ioamhram_[0x155] & 0x80) && lcd_.hdmaIsEnabled()) {
		if (lastOamDmaUpdate_ != disabled_time)
			updateOamDma(cc);

		lcd_.disableHdma(cc);
	}

	return cc;
}

void Memory::freeze(unsigned long cc) {
	// permanently halt CPU.
	// simply halt and clear IE to avoid unhalt from occuring,
	// which avoids additional state to represent a "frozen" state.
	nontrivial_ff_write(0xFF, 0, cc);
	ackDmaReq(intreq_);
	intreq_.halt();
}

bool Memory::halt(unsigned long cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	haltHdmaState_ = lcd_.hdmaIsEnabled() && lcd_.isHdmaPeriod(cc)
		? hdma_high : hdma_low;
	bool const hdmaReq = hdmaReqFlagged(intreq_);
	if (hdmaReq)
		haltHdmaState_ = hdma_requested;
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc + 4);

	ackDmaReq(intreq_);
	intreq_.halt();
	return hdmaReq;
}

unsigned Memory::pendingIrqs(unsigned long cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	updateIrqs(cc);
	return intreq_.pendingIrqs();
}

void Memory::ackIrq(unsigned bit, unsigned long cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	// TODO: adjust/extend IRQ assertion time rather than use the odd cc offsets?
	// NOTE: a minimum offset of 2 is required for the LCD due to implementation assumptions w.r.t. cc headroom.
	updateSerial(cc + 3 + isCgb());
	updateTimaIrq(cc + 2 + isCgb());
	lcd_.update(cc + 2);
	intreq_.ackIrq(bit);
}

unsigned long Memory::stop(unsigned long cc, bool &prefetched) {
	// FIXME: this is incomplete.
	intreq_.setEventTime<intevent_unhalt>(cc + 0x20000 + 4);

	// speed change.
	if (ioamhram_[0x14D] & (isCgb() & !isCgbDmg())) {
		tima_.speedChange(TimaInterruptRequester(intreq_));
		// DIV reset.
		nontrivial_ff_write(0x04, 0, cc);
		haltHdmaState_ = lcd_.hdmaIsEnabled() && lcd_.isHdmaPeriod(cc)
			? hdma_high : hdma_low;
		prefetched = hdmaReqFlagged(intreq_);
		if (prefetched && isDoubleSpeed())
			haltHdmaState_ = hdma_requested;
		unsigned long const cc_ = cc + 8 * !isDoubleSpeed();
		if (cc_ >= cc + 4) {
			if (lastOamDmaUpdate_ != disabled_time)
				updateOamDma(cc + 4);
			if (!prefetched || isDoubleSpeed())
				ackDmaReq(intreq_);
			intreq_.halt();
		}
		psg_.speedChange(cc_, isDoubleSpeed());
		lcd_.speedChange(cc_);
		cart_.speedChange(cc_);
		ioamhram_[0x14D] ^= 0x81;
		// TODO: perhaps make this a bit nicer?
		intreq_.setEventTime<intevent_blit>(ioamhram_[0x140] & lcdc_en
			? lcd_.nextMode1IrqTime()
			: cc + (lcd_cycles_per_frame << isDoubleSpeed()));
		if (intreq_.eventTime(intevent_end) > cc_) {
			intreq_.setEventTime<intevent_end>(cc_
				+ (isDoubleSpeed()
				   ? (intreq_.eventTime(intevent_end) - cc_) * 2
				   : (intreq_.eventTime(intevent_end) - cc_) / 2));
		}
		if (cc_ < cc + 4) {
			if (lastOamDmaUpdate_ != disabled_time)
				updateOamDma(cc + 4);
			if (!prefetched || !isDoubleSpeed())
				ackDmaReq(intreq_);
			intreq_.halt();
		}
		// ensure that no updates with a previous cc occur.
		cc += 8;
	} else {
		// FIXME: test and implement stop correctly.
		prefetched = halt(cc);
		cc += 4;

		// GSR NOTE: "make stop really stop" (commit 9a79253, since r649)
		stopped_ = true;
		intreq_.setEventTime<intevent_unhalt>(disabled_time);
	}

	return cc;
}

void Memory::stall(unsigned long cc, unsigned long cycles) {
	intreq_.halt();
	intreq_.setEventTime<intevent_unhalt>(cc + cycles);
}

void Memory::decEventCycles(IntEventId eventId, unsigned long dec) {
	if (intreq_.eventTime(eventId) != disabled_time)
		intreq_.setEventTime(eventId, intreq_.eventTime(eventId) - dec);
}

unsigned long Memory::resetCounters(unsigned long cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	updateIrqs(cc);

	unsigned long const dec = cc < 0x20000
		? 0
		: (cc & -0x10000l) - 0x10000;
	decCycles(lastOamDmaUpdate_, dec);
	decCycles(lastCartBusUpdate_, dec);
	decEventCycles(intevent_serial, dec);
	decEventCycles(intevent_oam, dec);
	decEventCycles(intevent_blit, dec);
	decEventCycles(intevent_end, dec);
	decEventCycles(intevent_unhalt, dec);

	unsigned long const oldCC = cc;
	cc -= dec;
	intreq_.resetCc(oldCC, cc);
	cart_.resetCc(oldCC, cc);
	tima_.resetCc(oldCC, cc, TimaInterruptRequester(intreq_));
	lcd_.resetCc(oldCC, cc);
	psg_.resetCounter(cc, oldCC, isDoubleSpeed());
	return cc;
}

void Memory::updateInput() {
	unsigned state = 0xF;

	if ((ioamhram_[0x100] & 0x30) != 0x30) {
		if (getInput_) {
			unsigned input = (*getInput_)(getInputP_);
			unsigned dpad_state = ~input >> 4;
			unsigned button_state = ~input;
			if (!(ioamhram_[0x100] & 0x10))
				state &= dpad_state;
			if (!(ioamhram_[0x100] & 0x20))
				state &= button_state;

			if (state != 0xF && (ioamhram_[0x100] & 0xF) == 0xF)
				intreq_.flagIrq(0x10);
		}
	} else if (isSgb())
		state -= getJoypadIndex();

	ioamhram_[0x100] = (ioamhram_[0x100] & -0x10u) | state;
}

void Memory::updateOamDma(unsigned long const cc) {
	unsigned char const *const oamDmaSrc = oamDmaSrcPtr();
	unsigned cycles = (cc - lastOamDmaUpdate_) >> 2;

	if (halted()) {
		lastOamDmaUpdate_ += 4 * cycles;
	} else while (cycles--) {
		oamDmaPos_ = (oamDmaPos_ + 1) & 0xFF;
		lastOamDmaUpdate_ += 4;
		if (oamDmaPos_ == oamDmaStartPos_)
			startOamDma(lastOamDmaUpdate_);

		if (oamDmaPos_ < oam_size) {
			if (oamDmaSrc) ioamhram_[oamDmaPos_] = oamDmaSrc[oamDmaPos_];
			else if (cart_.isHuC3()) ioamhram_[oamDmaPos_] = cart_.HuC3Read(oamDmaPos_, cc);
			else ioamhram_[oamDmaPos_] = cart_.rtcRead();
		} else if (oamDmaPos_ == oam_size) {
			endOamDma(lastOamDmaUpdate_);
			if (oamDmaStartPos_ == 0) {
				lastOamDmaUpdate_ = disabled_time;
				break;
			}
		}
	}
}

void Memory::oamDmaInitSetup() {
	if (ioamhram_[0x146] < mm_sram_begin / 0x100) {
		cart_.setOamDmaSrc(ioamhram_[0x146] < mm_vram_begin / 0x100 ? oam_dma_src_rom : oam_dma_src_vram);
	} else if (ioamhram_[0x146] < 0x100 - isCgb() * 0x20) {
		cart_.setOamDmaSrc(ioamhram_[0x146] < mm_wram_begin / 0x100 ? oam_dma_src_sram : oam_dma_src_wram);
	} else
		cart_.setOamDmaSrc(oam_dma_src_invalid);
}

unsigned char const * Memory::oamDmaSrcPtr() const {
	switch (cart_.oamDmaSrc()) {
	case oam_dma_src_rom:
		return cart_.romdata(ioamhram_[0x146] >> 6) + ioamhram_[0x146] * 0x100l;
	case oam_dma_src_sram:
		return cart_.rsrambankptr() ? cart_.rsrambankptr() + ioamhram_[0x146] * 0x100l : 0;
	case oam_dma_src_vram:
		return cart_.vrambankptr() + ioamhram_[0x146] * 0x100l;
	case oam_dma_src_wram:
		return cart_.wramdata(ioamhram_[0x146] >> 4 & 1) + (ioamhram_[0x146] * 0x100l & 0xFFF);
	case oam_dma_src_invalid:
	case oam_dma_src_off:
		break;
	}

	return cart_.rdisabledRam();
}

void Memory::startOamDma(unsigned long cc) {
	oamDmaPos_ = 0;
	oamDmaStartPos_ = 0;
	lcd_.oamChange(cart_.rdisabledRam(), cc);
}

void Memory::endOamDma(unsigned long cc) {
	if (oamDmaStartPos_ == 0) {
		oamDmaPos_ = -2u & 0xFF;
		cart_.setOamDmaSrc(oam_dma_src_off);
	}
	lcd_.oamChange(ioamhram_, cc);
}

unsigned Memory::nontrivial_ff_read(unsigned const p, unsigned long const cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	switch (p) {
	case 0x00:
		updateInput();
		break;
	case 0x01:
	case 0x02:
		updateSerial(cc);
		break;
	case 0x04:
		return (cc - tima_.divLastUpdate()) >> 8 & 0xFF;
	case 0x05:
		ioamhram_[0x105] = tima_.tima(cc);
		break;
	case 0x0F:
		updateIrqs(cc);
		ioamhram_[0x10F] = intreq_.ifreg();
		break;
	case 0x26:
		if (ioamhram_[0x126] & 0x80) {
			psg_.generateSamples(cc, isDoubleSpeed());
			ioamhram_[0x126] = 0xF0 | psg_.getStatus();
		} else
			ioamhram_[0x126] = 0x70;

		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		psg_.generateSamples(cc, isDoubleSpeed());
		return psg_.waveRamRead(p & 0xF);
	case 0x41:
		return ioamhram_[0x141] | lcd_.getStat(ioamhram_[0x145], cc);
	case 0x44:
		return lcd_.getLyReg(cc);
	case 0x4C:
		if (!biosMode_)
			return 0xFF;

		break;
	case 0x50:
		return 0xFE | !biosMode_;
	case 0x56:
		if (isCgb() && !isCgbDmg()) {
			if (!agbFlag_ && ((ioamhram_[0x156] & 0xC0) == 0xC0) && cart_.getIrSignal(linked_ ? Infrared::linked_gb : Infrared::remote, cc))
				return ioamhram_[0x156] & ~0x02;
		}

		return ioamhram_[0x156] | 0x02;
	case 0x69:
		if (isCgb() && !isCgbDmg())
			return lcd_.cgbBgColorRead(ioamhram_[0x168] & 0x3F, cc);

		break;
	case 0x6B:
		if (isCgb() && !isCgbDmg())
			return lcd_.cgbSpColorRead(ioamhram_[0x16A] & 0x3F, cc);

		break;
	case 0x76:
		if (isCgb())
			return psg_.isEnabled() ? psg_.pcm12Read(cc, isDoubleSpeed()) : 0;

		break;
	case 0x77:
		if (isCgb())
			return psg_.isEnabled() ? psg_.pcm34Read(cc, isDoubleSpeed()) : 0;

		break;
	default:
		break;
	}

	return ioamhram_[p + 0x100];
}

unsigned Memory::nontrivial_read(unsigned const p, unsigned long const cc) {
	if (p < mm_hram_begin) {
		if (lastOamDmaUpdate_ != disabled_time) {
			updateOamDma(cc);

			if (cart_.isInOamDmaConflictArea(p) && oamDmaPos_ < oam_size) {
				int const r = isCgb() && cart_.oamDmaSrc() != oam_dma_src_wram && p >= mm_wram_begin
					? cart_.wramdata(ioamhram_[0x146] >> 4 & 1)[p & 0xFFF]
					: ioamhram_[oamDmaPos_];
				if (isCgb() && cart_.oamDmaSrc() == oam_dma_src_vram)
					ioamhram_[oamDmaPos_] = 0;

				return r;
			}
		}

		if (p < mm_wram_begin) {
			if (p < mm_vram_begin)
				return cart_.romdata(p >> 14)[p];

			if (p < mm_sram_begin) {
				if (!lcd_.vramReadable(cc))
					return 0xFF;

				if (p < 0x9000) {
					if (lcd_.vramExactlyReadable(cc)) {
						return 0x00;
					}
				}

				return cart_.vrambankptr()[p];
			}

			if (cart_.rsrambankptr())
				return cart_.rsrambankptr()[p];

			if (cart_.disabledRam())
				return cartBus_;

			if (cart_.isHuC1())
				return 0xC0 | cart_.getIrSignal(linked_ ? Infrared::linked_gb : Infrared::remote, cc);

			if (cart_.isHuC3())
				return cart_.HuC3Read(p, cc);

			if (cart_.isPocketCamera())
				return cart_.cameraRead(p, cc);

			return cart_.rtcRead();
		}

		if (p < mm_oam_begin)
			return cart_.wramdata(p >> 12 & 1)[p & 0xFFF];

		long const ffp = static_cast<long>(p) - mm_io_begin;
		if (ffp >= 0)
			return nontrivial_ff_read(ffp, cc);

		if (!lcd_.oamReadable(cc) || oamDmaPos_ < oam_size)
			return 0xFF;

		if (p >= (mm_oam_begin + oam_size) && isCgb() && !agbFlag_)
			return ioamhram_[(p - mm_oam_begin) & 0xE7];
	}

	return ioamhram_[p - mm_oam_begin];
}

unsigned Memory::nontrivial_ff_peek(unsigned const p, unsigned long const cc) {
	// some regs might be wrong
	switch (p) {
	case 0x04:
		return (cc - tima_.divLastUpdate()) >> 8 & 0xFF;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		return psg_.waveRamRead(p & 0xF);
	case 0x44:
		return lcd_.peekLy();
	case 0x4C:
		if (!biosMode_)
			return 0xFF;

		break;
	case 0x50:
		return 0xFE | !biosMode_;
	case 0x56:
		if (isCgb() && !isCgbDmg()) {
			if (linked_ && !agbFlag_ && ((ioamhram_[0x156] & 0xC0) == 0xC0) && cart_.getIrSignal(Infrared::linked_gb, cc))
				return ioamhram_[0x156] & ~0x02;
		}

		return ioamhram_[0x156] | 0x02;
	default:
		break;
	}

	return ioamhram_[p + 0x100];
}

unsigned Memory::nontrivial_peek(unsigned const p, unsigned long const cc) {
	if (p < mm_hram_begin) {
		if (lastOamDmaUpdate_ != disabled_time) {
			if (cart_.isInOamDmaConflictArea(p) && oamDmaPos_ < oam_size) {
				int const r = isCgb() && cart_.oamDmaSrc() != oam_dma_src_wram && p >= mm_wram_begin
					? cart_.wramdata(ioamhram_[0x146] >> 4 & 1)[p & 0xFFF]
					: ioamhram_[oamDmaPos_];

				return r;
			}
		}

		if (p < mm_wram_begin) {
			if (p < mm_vram_begin)
				return cart_.romdata(p >> 14)[p];

			if (p < mm_sram_begin)
				return cart_.vrambankptr()[p];

			if (cart_.rsrambankptr())
				return cart_.rsrambankptr()[p];

			if (cart_.disabledRam())
				return cartBus_;

			if (cart_.isHuC1())
				return 0xC0 | (linked_ && cart_.getIrSignal(Infrared::linked_gb, cc));

			if (cart_.isHuC3() || cart_.isPocketCamera())
				return 0xFF; // unsafe to peek

			return cart_.rtcRead(); // safe to peek
		}

		if (p < mm_oam_begin)
			return cart_.wramdata(p >> 12 & 1)[p & 0xFFF];

		long const ffp = static_cast<long>(p) - mm_io_begin;
		if (ffp >= 0)
			return nontrivial_ff_peek(ffp, cc);

		if (oamDmaPos_ < oam_size)
			return 0xFF;

		if (p >= (mm_oam_begin + oam_size) && isCgb() && !agbFlag_)
			return ioamhram_[(p - mm_oam_begin) & 0xE7];
	}

	return ioamhram_[p - mm_oam_begin];
}

void Memory::nontrivial_ff_write(unsigned const p, unsigned data, unsigned long const cc) {
	if (lastOamDmaUpdate_ != disabled_time)
		updateOamDma(cc);

	switch (p & 0xFF) {
	case 0x00:
		if ((data ^ ioamhram_[0x100]) & 0x30) {
			if (isSgb()) {
				if ((((data ^ ioamhram_[0x100]) & 0x30) & data) && !biosMode_)
					sgb_.onJoypad(ioamhram_[0x100], data);
			}

			ioamhram_[0x100] = (ioamhram_[0x100] & ~0x30u) | (data & 0x30);
			updateInput();
		}

		return;
	case 0x01:
		updateSerial(cc);
		break;
	case 0x02:
		updateSerial(cc);
		serialCnt_ = 8;
		if ((data & 0x81) == 0x81) {
			intreq_.setEventTime<intevent_serial>(data & (isCgb() & !isCgbDmg()) * 2
				? cc - (cc - tima_.divLastUpdate()) % 8 + 0x10 * serialCnt_
				: cc - (cc - tima_.divLastUpdate()) % 0x100 + 0x200 * serialCnt_);
		} else
			intreq_.setEventTime<intevent_serial>(disabled_time);

		data |= 0x7E - (isCgb() & !isCgbDmg()) * 2;
		break;
	case 0x04:
		if (intreq_.eventTime(intevent_serial) != disabled_time
				&& intreq_.eventTime(intevent_serial) > cc) {
			unsigned long const t = intreq_.eventTime(intevent_serial);
			unsigned long const n = ioamhram_[0x102] & isCgb() * 2
				? t + (cc - t) % 8 - 2 * ((cc - t) & 4)
				: t + (cc - t) % 0x100 - 2 * ((cc - t) & 0x80);
			intreq_.setEventTime<intevent_serial>(std::max(cc, n));
		}
		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.divReset(isDoubleSpeed());
		tima_.divReset(cc, TimaInterruptRequester(intreq_));
		return;
	case 0x05:
		tima_.setTima(data, cc, TimaInterruptRequester(intreq_));
		break;
	case 0x06:
		tima_.setTma(data, cc, TimaInterruptRequester(intreq_));
		break;
	case 0x07:
		data |= 0xF8;
		tima_.setTac(data, cc, TimaInterruptRequester(intreq_), agbFlag_);
		break;
	case 0x0F:
		updateIrqs(cc + 1 + isDoubleSpeed());
		intreq_.setIfreg(0xE0 | data);
		return;
	case 0x10:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr10(data);
		data |= 0x80;
		break;
	case 0x11:
		if (!psg_.isEnabled()) {
			if (isCgb())
				return;

			data &= 0x3F;
		}

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr11(data);
		data |= 0x3F;
		break;
	case 0x12:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr12(data);
		break;
	case 0x13:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr13(data);
		return;
	case 0x14:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr14(data, isDoubleSpeed());
		data |= 0xBF;
		break;
	case 0x16:
		if (!psg_.isEnabled()) {
			if (isCgb())
				return;

			data &= 0x3F;
		}

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr21(data);
		data |= 0x3F;
		break;
	case 0x17:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr22(data);
		break;
	case 0x18:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr23(data);
		return;
	case 0x19:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr24(data, isDoubleSpeed());
		data |= 0xBF;
		break;
	case 0x1A:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr30(data, interrupter_.getPc());
		data |= 0x7F;
		break;
	case 0x1B:
		if (!psg_.isEnabled() && isCgb())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr31(data);
		return;
	case 0x1C:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr32(data);
		data |= 0x9F;
		break;
	case 0x1D:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr33(data);
		return;
	case 0x1E:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr34(data);
		data |= 0xBF;
		break;
	case 0x20:
		if (!psg_.isEnabled() && isCgb())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr41(data);
		return;
	case 0x21:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr42(data);
		break;
	case 0x22:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr43(data);
		break;
	case 0x23:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setNr44(data);
		data |= 0xBF;
		break;
	case 0x24:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.setSoVolume(data);
		break;
	case 0x25:
		if (!psg_.isEnabled())
			return;

		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.mapSo(data);
		break;
	case 0x26:
		if ((ioamhram_[0x126] ^ data) & 0x80) {
			psg_.generateSamples(cc, isDoubleSpeed());

			if (!(data & 0x80)) {
				for (unsigned i = 0x10; i < 0x26; ++i)
					ff_write<false>(i, 0, cc);

				psg_.setEnabled(false);
			} else {
				psg_.reset(isDoubleSpeed());
				psg_.setEnabled(true);
			}
		}

		data = (data & 0x80) | (ioamhram_[0x126] & 0x7F);
		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		psg_.generateSamples(cc, isDoubleSpeed());
		psg_.waveRamWrite(p & 0xF, data);
		break;
	case 0x40:
		if (ioamhram_[0x140] != data) {
			if ((ioamhram_[0x140] ^ data) & lcdc_en) {
				unsigned const stat = data & lcdc_en ? ioamhram_[0x141] : lcd_.getStat(ioamhram_[0x145], cc);
				bool const hdmaEnabled = lcd_.hdmaIsEnabled();

				lcd_.lcdcChange(data, cc);
				ioamhram_[0x144] = 0;
				ioamhram_[0x141] &= 0xF8;

				if (data & lcdc_en) {
					if (ioamhram_[0x141] & lcdstat_lycirqen && ioamhram_[0x145] == 0 && !(stat & lcdstat_lycflag))
						intreq_.flagIrq(2);

					intreq_.setEventTime<intevent_blit>(blanklcd_
						? lcd_.nextMode1IrqTime()
						: lcd_.nextMode1IrqTime() + (lcd_cycles_per_frame << isDoubleSpeed()));
				} else {
					ioamhram_[0x141] |= stat & lcdstat_lycflag;
					intreq_.setEventTime<intevent_blit>(
						cc + (lcd_cycles_per_line * 4 << isDoubleSpeed()));

					if (hdmaEnabled)
						flagHdmaReq(intreq_);
				}
			} else
				lcd_.lcdcChange(data, cc);

			ioamhram_[0x140] = data;
		}

		return;
	case 0x41:
		lcd_.lcdstatChange(data, cc);
		if (!(ioamhram_[0x140] & lcdc_en) && (ioamhram_[0x141] & lcdstat_lycflag)
				&& (~ioamhram_[0x141] & lcdstat_lycirqen & (isCgb() ? data : -1))) {
			intreq_.flagIrq(2);
		}
		data = (ioamhram_[0x141] & 0x87) | (data & 0x78);
		break;
	case 0x42:
		lcd_.scyChange(data, cc);
		break;
	case 0x43:
		lcd_.scxChange(data, cc);
		break;
	case 0x45:
		lcd_.lycRegChange(data, cc);
		break;
	case 0x46:
		lastOamDmaUpdate_ = cc;
		oamDmaStartPos_ = (oamDmaPos_ + 2) & 0xFF;
		intreq_.setEventTime<intevent_oam>(std::min(intreq_.eventTime(intevent_oam), cc + 8));
		ioamhram_[0x146] = data;
		oamDmaInitSetup();
		return;
	case 0x47:
		if (!isCgb() || isCgbDmg())
			lcd_.dmgBgPaletteChange(data, cc);

		break;
	case 0x48:
		if (!isCgb() || isCgbDmg())
			lcd_.dmgSpPalette1Change(data, cc);

		break;
	case 0x49:
		if (!isCgb() || isCgbDmg())
			lcd_.dmgSpPalette2Change(data, cc);

		break;
	case 0x4A:
		lcd_.wyChange(data, cc);
		break;
	case 0x4B:
		lcd_.wxChange(data, cc);
		break;
	case 0x4C:
		if (!biosMode_)
			return;

		break;
	case 0x4D:
		if (isCgb() && !isCgbDmg())
			ioamhram_[0x14D] = (ioamhram_[0x14D] & ~1u) | (data & 1);

		return;
	case 0x4F:
		if (isCgb() && !isCgbDmg()) {
			cart_.setVrambank(data & 1);
			ioamhram_[0x14F] = 0xFE | data;
		}

		return;
	case 0x50:
		if (!biosMode_)
			return;

		if (data & 1) {
			if (isCgb() && (ioamhram_[0x14C] & 0x04)) {
				lcd_.copyCgbPalettesToDmg();
				lcd_.dmgBgPaletteChange(ioamhram_[0x147], cc);
				lcd_.dmgSpPalette1Change(ioamhram_[0x148], cc);
				lcd_.dmgSpPalette2Change(ioamhram_[0x149], cc);
				lcd_.setCgbDmg(true);
				ioamhram_[0x102] |= 0x02;
				ioamhram_[0x14D] |= 0x81;
				ioamhram_[0x156] |= 0xC1;
				ioamhram_[0x16B] |= 0xFF;
				ioamhram_[0x170] |= 0x07;
				ioamhram_[0x174] |= 0xFF;
			}

			biosMode_ = false;
		}

		return;
	case 0x51:
		dmaSource_ = data << 8 | (dmaSource_ & 0xFF);
		return;
	case 0x52:
		dmaSource_ = (dmaSource_ & 0xFF00) | (data & 0xF0);
		return;
	case 0x53:
		dmaDestination_ = data << 8 | (dmaDestination_ & 0xFF);
		return;
	case 0x54:
		dmaDestination_ = (dmaDestination_ & 0xFF00) | (data & 0xF0);
		return;
	case 0x55:
		if (isCgb() && !isCgbDmg()) {
			ioamhram_[0x155] = data & 0x7F;

			if (lcd_.hdmaIsEnabled()) {
				if (!(data & 0x80)) {
					ioamhram_[0x155] |= 0x80;
					lcd_.disableHdma(cc);
				}
			} else {
				if (data & 0x80) {
					lcd_.enableHdma(cc);
				} else
					flagGdmaReq(intreq_);
			}
		}

		return;
	case 0x56:
		if (isCgb() && !isCgbDmg()) {
			if (!agbFlag_) {
				cart_.setIrSignal(Infrared::this_gb, data & 1);
				cart_.setRemoteActive((data & 0xC0) == 0xC0);
			}

			ioamhram_[0x156] = (data & 0xC1) | (ioamhram_[0x156] & 0x3E);
		}

		return;
	case 0x68:
		if (isCgb() && !isCgbDmg())
			ioamhram_[0x168] = data | 0x40;

		return;
	case 0x69:
		if (isCgb() && !isCgbDmg()) {
			unsigned index = ioamhram_[0x168] & 0x3F;
			lcd_.cgbBgColorChange(index, data, cc);
			ioamhram_[0x168] = (ioamhram_[0x168] & ~0x3Fu)
				| ((index + (ioamhram_[0x168] >> 7)) & 0x3F);
		}

		return;
	case 0x6A:
		if (isCgb() && !isCgbDmg())
			ioamhram_[0x16A] = data | 0x40;

		return;
	case 0x6B:
		if (isCgb() && !isCgbDmg()) {
			unsigned index = ioamhram_[0x16A] & 0x3F;
			lcd_.cgbSpColorChange(index, data, cc);
			ioamhram_[0x16A] = (ioamhram_[0x16A] & ~0x3Fu)
				| ((index + (ioamhram_[0x16A] >> 7)) & 0x3F);
		}

		return;
	case 0x6C:
		if (isCgb() && !isCgbDmg()) {
			if (biosMode_)
				lcd_.setSpPriority(data & 1, cc);

			ioamhram_[0x16C] = data | 0xFE;
		}

		return;
	case 0x70:
		if (isCgb() && !isCgbDmg()) {
			cart_.setWrambank(data & 0x07 ? data & 0x07 : 1);
			ioamhram_[0x170] = data | 0xF8;
		}

		return;
	case 0x72:
	case 0x73:
		if (isCgb())
			break;

		return;
	case 0x74:
		if (isCgb() && !isCgbDmg())
			break;

		return;
	case 0x75:
		if (isCgb())
			ioamhram_[0x175] = data | 0x8F;

		return;
	case 0xFF:
		intreq_.setIereg(data);
		break;
	default:
		return;
	}

	ioamhram_[p + 0x100] = data;
}

void Memory::nontrivial_write(unsigned const p, unsigned const data, unsigned long const cc) {
	if (lastOamDmaUpdate_ != disabled_time) {
		updateOamDma(cc);

		if (cart_.isInOamDmaConflictArea(p) && oamDmaPos_ < oam_size) {
			if (isCgb()) {
				if (p < mm_wram_begin)
					ioamhram_[oamDmaPos_] = cart_.oamDmaSrc() != oam_dma_src_vram ? data : 0;
				else if (cart_.oamDmaSrc() != oam_dma_src_wram)
					cart_.wramdata(ioamhram_[0x146] >> 4 & 1)[p & 0xFFF] = data;
			} else {
				ioamhram_[oamDmaPos_] = cart_.oamDmaSrc() == oam_dma_src_wram
					? ioamhram_[oamDmaPos_] & data
					: data;
			}

			return;
		}
	}

	if (p < mm_oam_begin) {
		if (p < mm_sram_begin) {
			if (p < mm_vram_begin) {
				cart_.mbcWrite(p, data, cc);
			} else if (lcd_.vramWritable(cc)) {
				lcd_.vramChange(cc);
				cart_.vrambankptr()[p] = data;
			}
		} else if (p < mm_wram_begin) {
			if (cart_.wsrambankptr())
				cart_.wsrambankptr()[p] = data;
			else if (cart_.isHuC1()) {
				cart_.setIrSignal(Infrared::this_gb, data & 1);
			} else if (cart_.isHuC3())
				cart_.HuC3Write(p, data, cc);
			else if (cart_.isPocketCamera())
				cart_.cameraWrite(p, data, cc);
			else
				cart_.rtcWrite(data, cc);
		} else
			cart_.wramdata(p >> 12 & 1)[p & 0xFFF] = data;
	} else if (p - mm_hram_begin >= 0x7Fu) {
		long const ffp = static_cast<long>(p) - mm_io_begin;
		if (ffp < 0) {
			bool const validOam = p < (mm_oam_begin + oam_size);
			if (lcd_.oamWritable(cc) && oamDmaPos_ >= oam_size
					&& (validOam || (isCgb() && !agbFlag_))) {
				lcd_.oamChange(cc);
				ioamhram_[(p - mm_oam_begin) & (validOam ? 0xFF : 0xE7)] = data;
			}
		} else
			nontrivial_ff_write(ffp, data, cc);
	} else
		ioamhram_[p - mm_oam_begin] = data;
}

LoadRes Memory::loadROM(Array<unsigned char> &buffer, unsigned flags, std::string const &filepath) {
	bool const cgbMode = flags & GB::LoadFlag::CGB_MODE;

	if (LoadRes const fail = cart_.loadROM(buffer, cgbMode, filepath))
		return fail;

	agbFlag_ = flags & GB::LoadFlag::GBA_FLAG;
	gbIsSgb_ = flags & GB::LoadFlag::SGB_MODE;

	psg_.init(cart_.isCgb(), agbFlag_);
	lcd_.reset(ioamhram_, cart_.vramdata(), cart_.isCgb(), agbFlag_);
	interrupter_.setGameShark(std::string());

	if (!filepath.empty() && agbFlag_ && (crc32(0, bios_.get(), bios_.size()) == 0x41884E46)) { // patch cgb bios to re'd agb bios equal
		bios_.get()[0xF3] ^= 0x03;
		for (unsigned i = 0xF5; i < 0xFB; i++)
			bios_.get()[i] = bios_.get()[i + 1];

		bios_.get()[0xFB] ^= 0x74;
	}

	return LOADRES_OK;
}

std::size_t Memory::fillSoundBuffer(unsigned long cc) {
	psg_.generateSamples(cc, isDoubleSpeed());
	std::size_t samples = psg_.fillBuffer();
	if (isSgb())
		sgb_.accumulateSamples(samples);

	if (cart_.isHuC3())
		cart_.accumulateSamples(cc);

	totalSamplesEmitted_ += samples;
	return samples;
}

bool Memory::getMemoryArea(int which, unsigned char **data, int *length) {
	if (!data || !length)
		return false;

	switch (which) {
	case 4: // oam
		*data = &ioamhram_[0];
		*length = 160;
		return true;
	case 5: // hram
		*data = &ioamhram_[384];
		*length = 127;
		return true;
	case 6: // bgpal
		*data = (unsigned char *)lcd_.bgPalette();
		*length = 32;
		return true;
	case 7: // sppal
		*data = (unsigned char *)lcd_.spPalette();
		*length = 32;
		return true;
	default: // pass to cartridge
		return cart_.getMemoryArea(which, data, length);
	}
}

int Memory::linkStatus(int which) {
	switch (which) {
	case 256: // ClockSignaled
		return linkClockTrigger_;
	case 257: // AckClockSignal
		linkClockTrigger_ = false;
		return 0;
	case 258: // GetOut
		return ioamhram_[0x101] & 0xFF;
	case 259: // InfraredSignaled
		return cart_.getIrTrigger();
	case 260: // AckInfraredSignal
		cart_.ackIrTrigger();
		return 0;
	case 261: // GetInfraredOut
		return cart_.getIrSignal(Infrared::this_gb, 0);
	case 262: // ShiftInOn
		cart_.setIrSignal(Infrared::linked_gb, true);
		return 0;
	case 263: // ShiftInOff
		cart_.setIrSignal(Infrared::linked_gb, false);
		return 0;
	case 264: // enable link connection
		linked_ = true;
		return 0;
	case 265: // disable link connection
		linked_ = false;
		return 0;
	default: // ShiftIn
		if (!(ioamhram_[0x102] & 0x01) || (ioamhram_[0x102] & 0x80)) { // was enabled
			ioamhram_[0x101] = which;
			ioamhram_[0x102] &= 0x7F;
			intreq_.flagIrq(8);
		}
		return 0;
	}

	return -1;
}

SYNCFUNC(Memory) {
	SSS(cart_);
	SSS(sgb_);
	NSS(ioamhram_);
	NSS(divLastUpdate_);
	NSS(lastOamDmaUpdate_);
	NSS(lastCartBusUpdate_);
	SSS(intreq_);
	SSS(tima_);
	SSS(lcd_);
	SSS(psg_);
	NSS(dmaSource_);
	NSS(dmaDestination_);
	NSS(oamDmaPos_);
	NSS(serialCnt_);
	NSS(cartBus_);
	NSS(blanklcd_);
	NSS(biosMode_);
	NSS(stopped_);
	NSS(linked_);
	NSS(linkClockTrigger_);
	NSS(totalSamplesEmitted_);
}
