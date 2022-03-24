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

Mbc5::Mbc5(MemPtrs &memptrs)
: memptrs_(memptrs)
, rombank_(1)
, rambank_(0)
, enableRam_(false)
{
}

bool Mbc5::disabledRam() const {
	return !enableRam_;
}

void Mbc5::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		enableRam_ = data == 0xA;
		setRambank();
		break;
	case 1:
		rombank_ = p < 0x3000
				 ? (rombank_  & 0x100) |  data
				 : (data << 8 & 0x100) | (rombank_ & 0xFF);
		setRombank();
		break;
	case 2:
		rambank_ = data & 0xF;
		setRambank();
		break;
	case 3:
		break;
	}
}

void Mbc5::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.rambank = rambank_;
	ss.enableRam = enableRam_;
}

void Mbc5::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	rambank_ = ss.rambank;
	enableRam_ = ss.enableRam;
	setRambank();
	setRombank();
}

void Mbc5::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(rambank_);
	NSS(enableRam_);
}

void Mbc5::setRambank() const {
	memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled,
						rambank_ & (rambanks(memptrs_) - 1));
}

void Mbc5::setRombank() const {
	memptrs_.setRombank(rombank_ & (rombanks(memptrs_) - 1));
}

}
