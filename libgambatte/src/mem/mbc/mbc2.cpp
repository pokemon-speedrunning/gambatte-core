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

Mbc2::Mbc2(MemPtrs &memptrs)
: memptrs_(memptrs)
, rombank_(1)
, enableRam_(false)
{
}

bool Mbc2::disabledRam() const {
	return !enableRam_;
}

void Mbc2::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p & 0x4100) {
	case 0x0000:
		enableRam_ = (data & 0xF) == 0xA;
		memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled, 0);
		break;
	case 0x0100:
		rombank_ = data & 0xF;
		memptrs_.setRombank(std::max((unsigned)rombank_, 1u) & (rombanks(memptrs_) - 1));
		break;
	}
}

void Mbc2::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.enableRam = enableRam_;
}

void Mbc2::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	enableRam_ = ss.enableRam;
	memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled, 0);
	memptrs_.setRombank(std::max((unsigned)rombank_, 1u) & (rombanks(memptrs_) - 1));
}

void Mbc2::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(enableRam_);
}

}
