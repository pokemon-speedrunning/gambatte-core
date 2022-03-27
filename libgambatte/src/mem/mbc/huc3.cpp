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

HuC3::HuC3(MemPtrs &memptrs, HuC3Chip *const huc3)
: memptrs_(memptrs)
, huc3_(huc3)
, rombank_(1)
, rambank_(0)
, ramflag_(0)
{
}

bool HuC3::disabledRam() const {
	return false;
}

void HuC3::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		ramflag_ = data & 0xF;
		setRambank();
		break;
	case 1:
		rombank_ = data;
		setRombank();
		break;
	case 2:
		rambank_ = data;
		setRambank();
		break;
	case 3:
		break;
	}
}

void HuC3::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.rambank = rambank_;
	ss.ramflag = ramflag_;
}

void HuC3::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	rambank_ = ss.rambank;
	ramflag_ = ss.ramflag;
	setRambank(false);
	setRombank();
}

void HuC3::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(rambank_);
	NSS(ramflag_);
}

void HuC3::setRambank(bool setRamflag) const {
	if (setRamflag)
		huc3_->setRamflag(ramflag_);

	unsigned flags;
	switch (ramflag_) {
		case 0x0: // read-only mode
			flags = MemPtrs::read_en;
			break;
		case 0xA: // read/write mode
			flags = MemPtrs::read_en | MemPtrs::write_en;
			break;
		case 0xB: // register write mode
		case 0xC: // register read mode
		case 0xD: // register commit mode
		case 0xE: // IR mode
			flags = MemPtrs::read_en | MemPtrs::write_en | MemPtrs::rtc_en;
			break;
		default: // disabled?
			flags = MemPtrs::disabled;
			break;
	}

	memptrs_.setRambank(flags, rambank_ & (rambanks(memptrs_) - 1));
}

void HuC3::setRombank() const {
	memptrs_.setRombank(std::max(rombank_ & (rombanks(memptrs_) - 1), 1u));
}

}
