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

#ifndef ENVELOPE_UNIT_H
#define ENVELOPE_UNIT_H

#include "sound_unit.h"
#include "../savestate.h"
#include "newstate.h"

namespace gambatte {

class EnvelopeUnit : public SoundUnit {
public:
	struct VolOnOffEvent {
		virtual ~VolOnOffEvent() {}
		virtual void operator()(unsigned long /*cc*/) {}
	};

	explicit EnvelopeUnit(VolOnOffEvent &volOnOffEvent = nullEvent_);
	void event();
	bool dacIsOn() const { return agb_ || (nr2_ & 0xF8); }
	unsigned getVolume() const { return volume_; }
	void nr2Change(unsigned newNr2, unsigned long cc, bool master);
	bool nr4Init(unsigned long cycleCounter);
	void reset();
	void init(bool agb) { agb_ = agb; }
	void saveState(SaveState::SPU::Env &estate) const;
	void loadState(SaveState::SPU::Env const &estate, unsigned nr2, unsigned long cc);
	template<bool isReader>void SyncState(NewState *ns);

private:
	static VolOnOffEvent nullEvent_;
	VolOnOffEvent &volOnOffEvent_;
	unsigned char nr2_;
	unsigned char volume_;
	bool clock_;
	bool agb_;

	bool clock(unsigned long cc) {
		if (counter_ == counter_disabled)
			return false;

		bool clock = (cc & 0x7800) == 0x1800 && clock_;
		clock_ = (counter_ % cc) & ~0x7FFF;
		return clock;
	}
};

}

#endif
