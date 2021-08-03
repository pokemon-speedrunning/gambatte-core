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

#ifndef CAMERA_H
#define CAMERA_H

#include "newstate.h"

namespace gambatte {

struct SaveState;

class Camera {
public:
	Camera();

	void saveState(SaveState &state) const;
	void loadState(SaveState const &state);

	void resetCc(unsigned long oldCc, unsigned long newCc);
	void speedChange(unsigned long cycleCounter);

	bool cameraIsActive(unsigned long cycleCounter);

	unsigned char read(unsigned p, unsigned long cycleCounter);
	void write(unsigned p, unsigned data, unsigned long cycleCounter);

	template<bool isReader>void SyncState(NewState *ns);

private:
	unsigned char trigger_;
	bool negative_;
	bool oldNegative_;
	unsigned short exposure_;
	unsigned short oldExposure_;
	unsigned long lastCycles_;
	long cameraCyclesLeft_;
	bool cancelled_;
	bool ds_;

	void update(unsigned long cycleCounter);
};

}

#endif
