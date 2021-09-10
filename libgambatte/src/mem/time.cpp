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
{
}

void Time::saveState(SaveState &state, unsigned long const cc, bool isHuC3) {
	if (isHuC3) {
		if (useCycles_)
			timeFromCycles(cc);
		else
			cyclesFromTime(cc);
	}
	state.time.seconds = seconds_;
	state.time.lastTimeSec = (isHuC3 || !useCycles_) ? lastTime_.tv_sec : 0;
	state.time.lastTimeUsec = (isHuC3 || !useCycles_) ? lastTime_.tv_usec : 0;
	state.time.lastCycles = lastCycles_;
}

void Time::loadState(SaveState const &state, bool isHuC3) {
	seconds_ = state.time.seconds;
	if (isHuC3 || !useCycles_) {
		lastTime_.tv_sec = state.time.lastTimeSec;
		lastTime_.tv_usec = state.time.lastTimeUsec;
	}
	lastCycles_ = state.time.lastCycles;
	ds_ = (!state.ppu.notCgbDmg) & state.mem.ioamhram.get()[0x14D] >> 7;
}

std::time_t Time::get(unsigned long const cc) {
	update(cc);
	return seconds_;
}

void Time::set(std::time_t seconds, unsigned long const cc) {
	update(cc);
	seconds_ = seconds;
}

void Time::reset(std::time_t seconds, unsigned long const cc) {
	set(seconds, cc);
	lastTime_ = now();
	lastCycles_ = cc;
}

void Time::resetCc(unsigned long const oldCc, unsigned long const newCc, bool isHuC3) {
	if (isHuC3)
		update(oldCc);

	lastCycles_ -= oldCc - newCc;
}

void Time::speedChange(unsigned long const cc, bool isHuC3) {
	if (isHuC3) {
		update(cc);
		if (useCycles_) {
			unsigned long diff = cc - lastCycles_;
			lastCycles_ = cc - (ds_ ? diff >> 1 : diff << 1);
		}
	}

	ds_ = !ds_;
}

timeval Time::baseTime(unsigned long const cc, bool isHuC3) {
	if (isHuC3) {
		if (useCycles_)
			timeFromCycles(cc);

		timeval baseTime = lastTime_;
		baseTime.tv_sec -= seconds_;
		return baseTime;
	} else
		return now();
}

void Time::setBaseTime(timeval baseTime, unsigned long const cc) {
	seconds_ = (now() - baseTime).tv_sec;
	lastTime_ = baseTime;
	lastTime_.tv_sec += seconds_;

	if (useCycles_)
		cyclesFromTime(cc);
}

void Time::setTimeMode(bool useCycles, unsigned long const cc, bool isHuC3) {
	if (useCycles != useCycles_) {
		if (isHuC3) {
			if (useCycles_)
				timeFromCycles(cc);
			else
				cyclesFromTime(cc);
		}
		useCycles_ = useCycles;
	}
}

unsigned Time::timeNow(unsigned long const cc) const {
	return (seconds_ * rtcDivisor_ + ((cc - lastCycles_) >> ds_)) >> 1;
}

void Time::update(unsigned long const cc) {
	if (useCycles_) {
		std::time_t diff = (cc - lastCycles_) / (rtcDivisor_ << ds_);
		seconds_ += diff;
		lastCycles_ += diff * (rtcDivisor_ << ds_);
	} else {
		std::time_t diff = (now() - lastTime_).tv_sec;
		seconds_ += diff;
		lastTime_.tv_sec += diff;
	}
}

void Time::cyclesFromTime(unsigned long const cc) {
	update(cc);
	timeval diff = now() - lastTime_;
	lastCycles_ = cc - diff.tv_usec * ((rtcDivisor_ << ds_) / 1000000.0f);
}

void Time::timeFromCycles(unsigned long const cc) {
	update(cc);
	unsigned long diff = cc - lastCycles_;
	timeval usec = { 0, (long)(diff / ((rtcDivisor_ << ds_) / 1000000.0f)) };
	lastTime_ = now() - usec;
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
	NSS(seconds_);
	NSS(lastTime_.tv_sec);
	NSS(lastTime_.tv_usec);
	NSS(lastCycles_);
	NSS(useCycles_);
	NSS(ds_);
}

}
