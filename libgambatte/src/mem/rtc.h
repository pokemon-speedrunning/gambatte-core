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

#ifndef RTC_H
#define RTC_H

#include "time.h"
#include "newstate.h"

namespace gambatte {

enum {
	rtc_dh,
	rtc_dl,
	rtc_h,
	rtc_m,
	rtc_s,
	rtc_c,
	rtc_l };

struct SaveState;

class Rtc : public Clock {
public:
	Rtc(Time &time);
	unsigned char const * activeLatch() const { return activeLatch_; }

	void latch(unsigned long const cc) { doLatch(cc); }

	void saveState(SaveState &state) const;
	void loadState(SaveState const &state);

	void getRtcRegs(unsigned long *dest, unsigned long const cc);
	void setRtcRegs(unsigned long *src);

	void set(bool enabled, unsigned bank) {
		bank &= 0xF;
		bank -= 8;

		enabled_ = enabled;
		index_ = bank;
		doSwapActive();
	}

	void write(unsigned data, unsigned long const cc) {
		(this->*activeSet_)(data, cc);
	}

	virtual void updateClock(unsigned long const cc);
	virtual void setTime(unsigned long long const dividers);
	virtual void setBaseTime(unsigned long long baseTime, unsigned long const cc);

	template<bool isReader>void SyncState(NewState *ns);

private:
	Time &time_;
	unsigned char *activeLatch_;
	void (Rtc::*activeSet_)(unsigned, unsigned long);
	unsigned char index_;
	bool enabled_;
	unsigned char dataDh_;
	unsigned char dataDl_;
	signed char dataH_;
	signed char dataM_;
	signed char dataS_;
	unsigned long dataC_;
	unsigned char latchDh_;
	unsigned char latchDl_;
	unsigned char latchH_;
	unsigned char latchM_;
	unsigned char latchS_;

	void doLatch(unsigned long cycleCounter);
	void doSwapActive();
	void setDh(unsigned newDh, unsigned long cycleCounter);
	void setDl(unsigned newLowdays, unsigned long cycleCounter);
	void setH(unsigned newHours, unsigned long cycleCounter);
	void setM(unsigned newMinutes, unsigned long cycleCounter);
	void setS(unsigned newSeconds, unsigned long cycleCounter);
};

}

#endif
