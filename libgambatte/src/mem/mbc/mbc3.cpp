//
//   Copyright (C) 2007-2010 by sinamas <sinamas at users.sourceforge.net>
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

#include "mbc.h"

namespace gambatte {

Mbc3::Mbc3(MemPtrs &memptrs, Rtc *const rtc, unsigned char rombankMask, unsigned char rambankMask)
: memptrs_(memptrs)
, rtc_(rtc)
, rombank_(1)
, rambank_(0)
, enableRam_(false)
, rombankMask_(rombankMask)
, rambankMask_(rambankMask)
{
}

bool Mbc3::disabledRam() const {
	return !enableRam_ || (rtc_ && ((rambank_ > (rambanks(memptrs_) - 1) && rambank_ < 0x08) || rambank_ > 0x0C));
}

void Mbc3::romWrite(unsigned const p, unsigned const data, unsigned long const cc) {
	switch (p >> 13 & 3) {
	case 0:
		enableRam_ = (data & 0xF) == 0xA;
		setRambank();
		break;
	case 1:
		rombank_ = data & rombankMask_;
		setRombank();
		break;
	case 2:
		{
			unsigned flags = MemPtrs::read_en | MemPtrs::write_en;
			rambank_ = data & (rtc_ ? 0x0F : rambankMask_);
			setRambank(flags);
		}
		break;
	case 3:
		if (rtc_)
			rtc_->latch(cc);

		break;
	}
}

void Mbc3::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.rambank = rambank_;
	ss.enableRam = enableRam_;
}

void Mbc3::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	rambank_ = ss.rambank;
	enableRam_ = ss.enableRam;
	setRambank();
	setRombank();
}

void Mbc3::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(rambank_);
	NSS(enableRam_);
}

void Mbc3::setRambank(unsigned flags) const {
	if (!enableRam_)
		flags = MemPtrs::disabled;

	if (rtc_) {
		if ((rambank_ > (rambanks(memptrs_) - 1) && rambank_ < 0x08) || rambank_ > 0x0C)
			flags = MemPtrs::disabled;

		rtc_->set(enableRam_, rambank_);

		if (rtc_->activeLatch())
			flags |= MemPtrs::rtc_en;
	}

	memptrs_.setRambank(flags, rambank_ & (rambanks(memptrs_) - 1));
}

void Mbc3::setRombank() const {
	memptrs_.setRombank(std::max((unsigned)rombank_, 1u) & (rombanks(memptrs_) - 1));
}

}
