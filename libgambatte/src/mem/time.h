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

#ifndef TIME_H
#define TIME_H

#include "newstate.h"

#include <ctime>
#include <sys/time.h>

namespace gambatte {

struct SaveState;

class Clock {
public:
	virtual void updateClock(unsigned long const cc);
	virtual unsigned timeNow() const;
	virtual void setBaseTime(timeval baseTime, unsigned long const cc);
};

class Time {
public:
	static timeval now() {
		timeval t;
		gettimeofday(&t, 0);
		return t;
	}

	Time();
	void saveState(SaveState &state, unsigned long cycleCounter);
	void loadState(SaveState const &state, bool cgb);

	void resetCc(unsigned long oldCc, unsigned long newCc);
	void speedChange(unsigned long cycleCounter);

	void setTimeMode(bool useCycles, unsigned long cycleCounter);

	void set(Clock *clock) { clock_ = clock; }
	unsigned timeNow() const { return clock_ ? clock_->timeNow() : 0; }
	void setBaseTime(timeval baseTime, unsigned long const cycleCounter) { if (clock_) clock_->setBaseTime(baseTime, cycleCounter); }

	unsigned long getRtcDivisor() { return rtcDivisor_; }
	void setRtcDivisorOffset(long const rtcDivisorOffset) { rtcDivisor_ = 0x400000L + rtcDivisorOffset; }	

	unsigned long diff(unsigned long cycleCounter);

	template<bool isReader>void SyncState(NewState *ns);

private:
	timeval lastTime_;
	unsigned long lastCycles_;
	bool useCycles_;
	unsigned long rtcDivisor_;
	bool ds_;

	Clock *clock_;
	void update(unsigned long const cc) { if (clock_) clock_->updateClock(cc); }
};

}

#endif
