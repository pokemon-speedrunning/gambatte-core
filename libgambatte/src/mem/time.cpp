//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#include "time.h"
#include "../savestate.h"

namespace gambatte {

Time::Time()
: lastTime_(std::time(0))
, lastCycles_(0)
: useCycles_(true)
, rtcDivisor_(0x400000)
, ds_(0)
, clock_(0)
{
}

void Time::saveState(SaveState &state, unsigned long const cc) {
	update(cc);
	state.time.seconds = 0; // old total seconds field, 0 indicates new time system
	// other fields used to be saved, but they no longer need to be saved
}

void Time::loadState(SaveState const &state, bool const ds) {
	lastCycles_ = state.cpu.cycleCounter;
	ds_ = ds;

	// state backwards compat with old time system
	if (state.time.seconds)
		setTime(state.time.seconds * rtcDivisor_ / 2);
}

void Time::resetCc(unsigned long const oldCc, unsigned long const newCc) {
	update(oldCc);
	lastCycles_ -= oldCc - newCc;
}

void Time::speedChange(unsigned long const cc) {
	update(cc);
	ds_ = !ds_;
}

void Time::setTimeMode(bool useCycles, unsigned long const cc) {
	update(cc);
	useCycles_ = useCycles;
	lastCycles_ = cc;
	lastTime_ = std::time(0);
}

unsigned long long Time::diff(unsigned long const cc) {
	unsigned long long diff;
	if (useCycles_) {
		diff = (cc - lastCycles_) >> ds_;
		lastCycles_ = cc;
	} else {
		std::time_t const now = std::time(0);
		diff = (now - lastTime_) * (unsigned long long)rtcDivisor_;
		lastTime_ = now;
	}
	return diff;
}

SYNCFUNC(Time) {
	NSS(lastCycles_);
	NSS(useCycles_);
	NSS(ds_);
}

}
