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

#include "camera.h"
#include "../savestate.h"

namespace gambatte {

Camera::Camera()
: trigger_(0)
, negative_(false)
, oldNegative_(false)
, exposure_(0)
, oldExposure_(0)
, cameraCyclesLeft_(0)
, cancelled_(false)
{
}

void Camera::saveState(SaveState &state) const {
	state.camera.trigger = trigger_;
	state.camera.negative = negative_;
	state.camera.oldNegative = oldNegative_;
	state.camera.exposure = exposure_;
	state.camera.oldExposure = oldExposure_;
	state.camera.lastCycles = lastCycles_;
	state.camera.cameraCyclesLeft = cameraCyclesLeft_;
	state.camera.cancelled = cancelled_;
}

void Camera::loadState(SaveState const &state) {
	trigger_ = state.camera.trigger;
	negative_ = state.camera.negative;
	oldNegative_ = state.camera.oldNegative;
	exposure_ = state.camera.exposure;
	oldExposure_ = state.camera.oldExposure;
	lastCycles_ = state.camera.lastCycles;
	cameraCyclesLeft_ = state.camera.cameraCyclesLeft;
	cancelled_ = state.camera.cancelled;
	ds_ = (!state.ppu.notCgbDmg) & state.mem.ioamhram.get()[0x14D] >> 7;
}

void Camera::resetCc(unsigned long const newCc, unsigned long const oldCc) {
	update(oldCc);
	lastCycles_ -= oldCc - newCc;
}

void Camera::speedChange(unsigned long const cc) {
	update(cc);
	ds_ = !ds_;
}

bool Camera::cameraIsActive(unsigned long const cc) {
	update(cc);
	return trigger_ & 0x01;
}

unsigned char Camera::read(unsigned p, unsigned long const cc) {
	if (p & 0x7F)
		return 0x00;

	update(cc);
	return trigger_;
}

void Camera::write(unsigned p, unsigned data, unsigned long const cc) {
	switch (p & 0x7F) {
	case 0x00:
	{
		bool cameraIsActive_ = cameraIsActive(cc);
		if ((data & 0x01) ^ cameraIsActive_) {
			if (cameraIsActive_) {
				cameraCyclesLeft_ = 0;
				oldNegative_ = negative_;
				oldExposure_ = exposure_;
				cancelled_ = true;
			} else {
				cameraCyclesLeft_ = cancelled_
					? 129784 + (!oldNegative_ * 512) + (oldExposure_ << 4)
					: 129784 + (   !negative_ * 512) + (   exposure_ << 4);
				lastCycles_ = cc;
			}
		}
		trigger_ = data & 0x07;
		break;
	}
	case 0x01:
		negative_ = data & 0x80;
		break;
	case 0x02:
		exposure_ &= 0xFF;
		exposure_ |= (data << 8) & 0xFF00;
		break;
	case 0x03:
		exposure_ &= 0xFF00;
		exposure_ |= data & 0xFF;
		break;
	default:
		// other regs unimplemented
		break;
	}
}

void Camera::update(unsigned long const cc) {
	if (cameraCyclesLeft_) {
		cameraCyclesLeft_ -= (cc - lastCycles_) >> ds_;
		lastCycles_ = cc;
		if (cameraCyclesLeft_ <= 0) {
			trigger_ &= 0xFE;
			cameraCyclesLeft_ = 0;
			cancelled_ = false;
		}
	}
}

SYNCFUNC(Camera) {
	NSS(trigger_);
	NSS(negative_);
	NSS(oldNegative_);
	NSS(exposure_);
	NSS(oldExposure_);
	NSS(lastCycles_);
	NSS(cameraCyclesLeft_);
	NSS(cancelled_);
	NSS(ds_);
}

}
