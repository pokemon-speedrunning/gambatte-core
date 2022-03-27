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

HuC1::HuC1(MemPtrs &memptrs, Infrared *const ir)
: memptrs_(memptrs)
, ir_(ir)
, rombank_(1)
, rambank_(0)
, ramflag_(0)
{
}

bool HuC1::disabledRam() const {
	return ramflag_ != 0x0 && ramflag_ != 0xA && ramflag_ != 0xE;
}

void HuC1::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		ramflag_ = data & 0xF;
		setRambank();
		break;
	case 1:
		rombank_ = data & 0x3F;
		setRombank();
		break;
	case 2:
		rambank_ = data & 3;
		setRambank();
		break;
	case 3:
		break;
	}
}

void HuC1::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.rambank = rambank_;
	ss.ramflag = ramflag_;
}

void HuC1::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	rambank_ = ss.rambank;
	ramflag_ = ss.ramflag;
	setRambank();
	setRombank();
}

void HuC1::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(rambank_);
	NSS(ramflag_);
}

void HuC1::setRambank() const {
	unsigned flags;
	switch (ramflag_) {
		case 0x0: // read-only mode
			flags = MemPtrs::read_en;
			ir_->setIrSignal(Infrared::this_gb, false);
			break;
		case 0xA: // read/write mode
			flags = MemPtrs::read_en | MemPtrs::write_en;
			ir_->setIrSignal(Infrared::this_gb, false);
			break;
		case 0xE: // IR mode
			flags = MemPtrs::read_en | MemPtrs::write_en | MemPtrs::rtc_en;
			break;
		default: // disabled?
			flags = MemPtrs::disabled;
			ir_->setIrSignal(Infrared::this_gb, false);
			break;
	}

	memptrs_.setRambank(flags, rambank_ & (rambanks(memptrs_) - 1));
}

void HuC1::setRombank() const {
	memptrs_.setRombank(rombank_ & (rombanks(memptrs_) - 1));
}

}
