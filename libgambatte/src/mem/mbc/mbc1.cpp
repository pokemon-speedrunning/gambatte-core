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

Mbc1::Mbc1(MemPtrs &memptrs, unsigned char rombankMask, unsigned char rombankShift)
: memptrs_(memptrs)
, enableRam_(false)
, rambankMode_(false)
, bankReg1_(1)
, bankReg2_(0)
, rombankMask_(rombankMask)
, rombankShift_(rombankShift)
{
	updateBanking();
}

inline void Mbc1::updateBanking() {
	rombank0_ = (bankReg2_ << rombankShift_) * rambankMode_;
	rombank_ = (bankReg2_ << rombankShift_) | bankReg1_;
	rambank_ = rambankMode_ * bankReg2_;
	setRambank();
	setRombank();
}

bool Mbc1::disabledRam() const {
	return !enableRam_;
}

void Mbc1::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		enableRam_ = (data & 0xF) == 0xA;
		setRambank();
		break;
	case 1:
		bankReg1_ = data & 0x1F ? data & rombankMask_ : 1;
		updateBanking();
		break;
	case 2:
		bankReg2_ = data & 3;
		updateBanking();
		break;
	case 3:
		rambankMode_ = data & 1;
		updateBanking();
		break;
	}
}

// fixme: this is kinda hacky but it works i guess
void Mbc1::saveState(SaveState::Mem &ss) const {
	ss.rombank = bankReg1_;
	ss.rambank = bankReg2_;
	ss.enableRam = enableRam_;
	ss.rambankMode = rambankMode_;
}

void Mbc1::loadState(SaveState::Mem const &ss) {
	bankReg1_ = ss.rombank;
	bankReg2_ = ss.rambank;
	enableRam_ = ss.enableRam;
	rambankMode_ = ss.rambankMode;
	updateBanking();
}

void Mbc1::SyncState(NewState *ns, bool isReader) {
	NSS(rombank0_);
	NSS(rombank_);
	NSS(rambank_);
	NSS(enableRam_);
	NSS(rambankMode_);
	NSS(bankReg1_);
	NSS(bankReg2_);
}

bool Mbc1::isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
	return (addr < 0x4000) == ((bank & rombankMask_) == 0);
}

void Mbc1::setRambank() const {
	memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled,
	                    rambank_ & (rambanks(memptrs_) - 1));
}

void Mbc1::setRombank() const {
	memptrs_.setRombank0(rombank0_ & (rombanks(memptrs_) - 1));
	memptrs_.setRombank(rombank_ & (rombanks(memptrs_) - 1));
}

}
