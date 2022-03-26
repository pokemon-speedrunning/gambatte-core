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

static timeval operator-(timeval l, timeval r) {
	timeval t;
	t.tv_sec = l.tv_sec - r.tv_sec;
	t.tv_usec = l.tv_usec - r.tv_usec;
	if (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	return t;
}

Time::Time()
: useCycles_(true)
, rtcDivisor_(0x400000)
, clock_(NULL)
{
}

void Time::saveState(SaveState &state, unsigned long const cc) {
	update(cc);
	state.time.seconds = 0;
	state.time.lastTimeSec = !useCycles_ ? lastTime_.tv_sec : 0;
	state.time.lastTimeUsec = !useCycles_ ? lastTime_.tv_usec : 0;
	state.time.lastCycles = lastCycles_;
}

void Time::loadState(SaveState const &state, bool cgb) {
	if (state.time.seconds) {
		// todo: savestate compatibility hax
	}
	if (!useCycles_) {
		lastTime_.tv_sec = state.time.lastTimeSec;
		lastTime_.tv_usec = state.time.lastTimeUsec;
	}
	lastCycles_ = state.time.lastCycles;
	ds_ = (cgb && state.ppu.notCgbDmg) & state.mem.ioamhram.get()[0x14D] >> 7;
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
}

unsigned long Time::diff(unsigned long const cc) {
	unsigned long diff_;
	timeval now_ = now();
	if (useCycles_) {
		unsigned long diff = (cc - lastCycles_) >> ds_;
		diff_ = diff;
	} else {
		timeval diff = { now_.tv_sec - lastTime_.tv_sec, 0 };
		diff_ = diff.tv_sec * rtcDivisor_;
	}
	lastCycles_ = cc;
	lastTime_ = now_;
	return diff_;
}

SYNCFUNC(Time) {
	NSS(lastTime_.tv_sec);
	NSS(lastTime_.tv_usec);
	NSS(lastCycles_);
	NSS(useCycles_);
	NSS(ds_);
}

}
