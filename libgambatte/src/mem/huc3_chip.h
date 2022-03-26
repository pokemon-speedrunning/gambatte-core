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

#ifndef HuC3Chip_H
#define HuC3Chip_H

#include "time.h"

namespace gambatte {

struct SaveState;

class HuC3Chip : public Clock {
public:
	HuC3Chip(Time &time);

	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state) const;
	void loadState(SaveState const &state);

	void setRamflag(unsigned char ramflag) {
		ramflag_ = ramflag;
		committing_ = ramflag == 0xD;
		irReceivingPulse_ = false;
	}

	bool isHuC3() const { return enabled_; }
	void set(bool enabled) { enabled_ = enabled; }

	void getHuC3Regs(unsigned char *dest, unsigned long const cc);
	void setHuC3Regs(unsigned char *src);

	unsigned char read(unsigned p, unsigned long const cc);
	void write(unsigned p, unsigned data, unsigned long cycleCounter);
	template<bool isReader>void SyncState(NewState *ns);

	virtual void updateClock(unsigned long const cc);
	virtual unsigned timeNow() const;
	virtual void setBaseTime(timeval baseTime, unsigned long const cc);

private:
	Time &time_;
	unsigned char io_[0x100];
	unsigned char ioIndex_;
	unsigned char transferValue_;
	unsigned char ramflag_;
	unsigned long irBaseCycle_;
	unsigned long rtcCycles_;
	bool enabled_;
	bool committing_;
	bool highIoReadOnly_;
	bool irReceivingPulse_;
};

}

#endif
