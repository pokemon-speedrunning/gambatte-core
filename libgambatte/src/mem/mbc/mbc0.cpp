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

Mbc0::Mbc0(MemPtrs &memptrs)
: memptrs_(memptrs)
, enableRam_(false)
{
}

bool Mbc0::disabledRam() const {
	return !enableRam_;
}

void Mbc0::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	if (p < rambank_size()) {
		enableRam_ = (data & 0xF) == 0xA;
		memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled, 0);
	}
}

void Mbc0::saveState(SaveState::Mem &ss) const {
	ss.enableRam = enableRam_;
}

void Mbc0::loadState(SaveState::Mem const &ss) {
	enableRam_ = ss.enableRam;
	memptrs_.setRambank(enableRam_ ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled, 0);
}

void Mbc0::SyncState(NewState *ns, bool isReader) {
	NSS(enableRam_);
}

}
