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

WisdomTree::WisdomTree(MemPtrs &memptrs)
: memptrs_(memptrs)
, rombank_(0)
{
}

bool WisdomTree::disabledRam() const {
	return true;
}

void WisdomTree::romWrite(unsigned const p, unsigned const /*data*/, unsigned long const /*cc*/) {
	rombank_ = (p & 0xFF) << 1;
	setRombank();
}

void WisdomTree::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
}

void WisdomTree::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	setRombank();
}

bool WisdomTree::isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
	return ((addr < rombank_size()) == !(bank & 1)) == ((addr >= rombank_size()) == (bank & 1));
}

void WisdomTree::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
}

void WisdomTree::setRombank() const {
	memptrs_.setRombank0(rombank_ & (rombanks(memptrs_) - 2));
	memptrs_.setRombank((rombank_ | 1) & (rombanks(memptrs_) - 1));
}

}
