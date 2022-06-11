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

M161::M161(MemPtrs& memptrs)
: memptrs_(memptrs)
, rombank_(0)
, mapped_(false)
{
}

bool M161::disabledRam() const {
	return true;
}

void M161::romWrite(unsigned const /*p*/, unsigned const data, unsigned long const /*cc*/) {
	if (!mapped_) {
		rombank_ = (data & 7) << 1;
		mapped_ = true;
		setRombank();
	}
}

void M161::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.enableRam = mapped_;
}

void M161::loadState(SaveState::Mem const& ss) {
	rombank_ = ss.rombank;
	mapped_ = ss.enableRam;
	setRombank();
}

bool M161::isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
	return ((addr < rombank_size()) == !(bank & 1)) == ((addr >= rombank_size()) == (bank & 1));
}

void M161::SyncState(NewState* ns, bool isReader) {
	NSS(rombank_);
	NSS(mapped_);
}

void M161::setRombank() const {
	memptrs_.setRombank0(rombank_ & (rombanks(memptrs_) - 2));
	memptrs_.setRombank((rombank_ | 1) & (rombanks(memptrs_) - 1));
}

}
