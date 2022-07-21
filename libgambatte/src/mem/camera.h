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

#include <algorithm>
#include <cstdint>

namespace gambatte {

struct SaveState;

class Camera {
public:
	Camera();

	void set(unsigned char *cameraRam) { cameraRam_ = cameraRam; }

	void setStatePtrs(SaveState &);
	void saveState(SaveState &state) const;
	void loadState(SaveState const &state, bool cgb);

	void resetCc(unsigned long oldCc, unsigned long newCc);
	void speedChange(unsigned long cycleCounter);

	bool cameraIsActive(unsigned long cycleCounter);

	unsigned char read(unsigned p, unsigned long cycleCounter);
	void write(unsigned p, unsigned data, unsigned long cycleCounter);

	void setCameraCallback(void(*callback)(uint32_t *cameraBuf)) { cameraCallback_ = callback; }

	template<bool isReader>void SyncState(NewState *ns);

private:
	unsigned char * cameraRam_;
	uint32_t cameraBuf_[128 * 112];

	unsigned char trigger_;
	bool n_;
	unsigned char vh_;
	unsigned short exposure_;
	unsigned char edgeAlpha_;
	bool blank_;
	bool invert_;
	unsigned char matrix_[4 * 4 * 3];

	unsigned long lastCycles_;
	long cameraCyclesLeft_;
	bool ds_;

	void update(unsigned long cycleCounter);
	void process();
	void (*cameraCallback_)(uint32_t *cameraBuf);
};

}

#endif
