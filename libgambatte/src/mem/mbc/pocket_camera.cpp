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

PocketCamera::PocketCamera(MemPtrs &memptrs, Camera *const camera)
: memptrs_(memptrs)
, camera_(camera)
, rombank_(1)
, rambank_(0)
, enableRam_(false)
{
	camera_->set(rambanks(memptrs_) ? &memptrs_.rambankdata()[0x100] : NULL);
}

bool PocketCamera::disabledRam() const {
	return false;
}

void PocketCamera::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		enableRam_ = (data & 0xF) == 0xA;
		setRambank();
		break;
	case 1:
		rombank_ = data & 0x3F;
		setRombank();
		break;
	case 2:
		rambank_ = data & 0x1F;
		setRambank();
		break;
	}
}

void PocketCamera::saveState(SaveState::Mem &ss) const {
	ss.rombank = rombank_;
	ss.rambank = rambank_;
	ss.enableRam = enableRam_;
}

void PocketCamera::loadState(SaveState::Mem const &ss) {
	rombank_ = ss.rombank;
	rambank_ = ss.rambank;
	enableRam_ = ss.enableRam;
	setRambank();
	setRombank();
	camera_->set(rambanks(memptrs_) ? &memptrs_.rambankdata()[0x100] : NULL);
}

void PocketCamera::SyncState(NewState *ns, bool isReader) {
	NSS(rombank_);
	NSS(rambank_);
	NSS(enableRam_);
	camera_->set(rambanks(memptrs_) ? &memptrs_.rambankdata()[0x100] : NULL);
}

void PocketCamera::setRambank() const {
	unsigned flags = MemPtrs::read_en;
	if (rambank_ & 0x10)
		flags |= MemPtrs::write_en | MemPtrs::rtc_en;

	if (enableRam_)
		flags |= MemPtrs::write_en;

	memptrs_.setRambank(flags, rambank_ & (rambanks(memptrs_) - 1));
}

void PocketCamera::setRombank() const {
	memptrs_.setRombank(rombank_ & (rombanks(memptrs_) - 1));
}

}
