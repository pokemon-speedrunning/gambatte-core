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

#ifndef TAMA6_H
#define TAMA6_H

#include "time.h"

namespace gambatte {

struct SaveState;

class Tama6 : public Clock {
public:
	Tama6(Time &time);

	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state) const;
	void loadState(SaveState const &state, bool const ds);

	void getHuC3Regs(unsigned char *dest, unsigned long cycleCounter);
	void setHuC3Regs(unsigned char *src);

	void resetCc(unsigned long oldCc, unsigned long newCc);
	void speedChange(unsigned long cycleCounter);

	void accumulateSamples(unsigned long cycleCounter);
	unsigned generateSamples(short *soundBuf);

	unsigned char read(unsigned p, unsigned long const cc);
	void write(unsigned p, unsigned data, unsigned long cycleCounter);
	template<bool isReader>void SyncState(NewState *ns);

	virtual void updateClock(unsigned long const cc);
	virtual unsigned long long timeNow() const;
	virtual void setTime(unsigned long long const dividers);
	virtual void setBaseTime(timeval baseTime, unsigned long const cc);

private:
	Time &time_;
	unsigned char rom_[0x800];
	unsigned char ram_[0x80];
	unsigned short pc;
	unsigned char h, l, a;
	unsigned zf, cf, sf, gf;
	unsigned char eir, il;
	bool eif;
	unsigned long cycleCounter_;

	unsigned readPort(unsigned p, unsigned long cc);
	void writePort(unsigned p, unsigned data, unsigned long cc);
	void runFor(unsigned long cycles);
};

}

#endif
