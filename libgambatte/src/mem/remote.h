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

#ifndef REMOTE_H
#define REMOTE_H

#include "savestate.h"
#include "newstate.h"
#include "../counterdef.h"

namespace gambatte {

class Remote {
public:
	Remote();
	bool getRemoteSignal(unsigned long const cc);

	void setRemoteActive(bool active) {
		if (isActive_ ^ active) {
			isActive_ = active;
			if (active) {
				lastUpdate_ = disabled_time;
				command_ = (remoteCallback_ ? remoteCallback_() : 0xFF) & 0x7F;
			}
		}
	}

	void setRemoteCallback(unsigned char (*callback)()) { remoteCallback_ = callback; }

	void resetCc(unsigned long const oldCc, unsigned long const newCc);
	void speedChange(unsigned long const cc);

	void saveState(SaveState::Infrared &state) const;
	void loadState(SaveState::Infrared const &state, bool const ds);
	template<bool isReader>void SyncState(NewState *ns);

private:
	bool isActive_;
	unsigned long lastUpdate_;
	unsigned long cyclesElapsed_;
	unsigned char command_;
	bool ds_;

	unsigned char (*remoteCallback_)();
	void update(unsigned long const cc);
};

}

#endif
