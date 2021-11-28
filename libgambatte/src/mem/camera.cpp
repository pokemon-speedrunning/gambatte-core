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

// most of this is copied from https://github.com/AntonioND/giibiiadvance/blob/master/source/gb_core/camera.c

#include "camera.h"
#include "../savestate.h"

namespace gambatte {

Camera::Camera()
: cameraRam_(NULL)
, trigger_(0)
, n_(false)
, vh_(0)
, exposure_(0)
, edgeAlpha_(0.50)
, blank_(false)
, invert_(false)
, oldTrigger_(0)
, oldN_(false)
, oldVh_(0)
, oldExposure_(0)
, oldEdgeAlpha_(0.50)
, oldBlank_(false)
, oldInvert_(false)
, cameraCyclesLeft_(0)
, cancelled_(false)
, ds_(false)
{
	std::memset(cameraBuf_, 0x00, sizeof cameraBuf_);
	std::memset(   matrix_, 0x00, sizeof    matrix_);
	std::memset(oldMatrix_, 0x00, sizeof oldMatrix_);
}

void Camera::setStatePtrs(SaveState &state) {
	state.camera.matrix.set   (   matrix_, sizeof    matrix_);
	state.camera.oldMatrix.set(oldMatrix_, sizeof oldMatrix_);
}

void Camera::saveState(SaveState &state) const {
	state.camera.trigger = trigger_;
	state.camera.n = n_;
	state.camera.vh = vh_;
	state.camera.exposure = exposure_;
	state.camera.edgeAlpha = edgeAlpha_ * 4;
	state.camera.blank = blank_;
	state.camera.invert = invert_;
	state.camera.oldTrigger = oldTrigger_;
	state.camera.oldN = oldN_;
	state.camera.oldVh = oldVh_;
	state.camera.oldExposure = oldExposure_;
	state.camera.oldEdgeAlpha = oldEdgeAlpha_ * 4;
	state.camera.oldBlank = oldBlank_;
	state.camera.oldInvert = oldInvert_;
	state.camera.lastCycles = lastCycles_;
	state.camera.cameraCyclesLeft = cameraCyclesLeft_;
	state.camera.cancelled = cancelled_;
}

void Camera::loadState(SaveState const &state) {
	trigger_ = state.camera.trigger;
	n_ = state.camera.n;
	vh_ = state.camera.vh;
	exposure_ = state.camera.exposure;
	edgeAlpha_ = state.camera.edgeAlpha / 4.0;
	blank_ = state.camera.blank;
	invert_ = state.camera.invert;
	oldTrigger_ = state.camera.oldTrigger;
	oldN_ = state.camera.oldN;
	oldVh_ = state.camera.oldVh;
	oldExposure_ = state.camera.oldExposure;
	oldEdgeAlpha_ = state.camera.oldEdgeAlpha / 4.0;
	oldBlank_ = state.camera.oldBlank;
	oldInvert_ = state.camera.oldInvert;
	lastCycles_ = state.camera.lastCycles;
	cameraCyclesLeft_ = state.camera.cameraCyclesLeft;
	cancelled_ = state.camera.cancelled;
	ds_ = state.ppu.notCgbDmg & state.mem.ioamhram.get()[0x14D] >> 7;
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
		bool active = cameraIsActive(cc);
		if ((data & 0x01) ^ active) {
			if (active) {
				cameraCyclesLeft_ = 0;
				oldTrigger_ = trigger_;
				oldN_ = n_;
				oldVh_ = vh_;
				oldExposure_ = exposure_;
				oldEdgeAlpha_ = edgeAlpha_;
				oldBlank_ = blank_;
				oldInvert_ = invert_;
				cancelled_ = true;
			} else {
				cameraCyclesLeft_ = cancelled_
					? 129792 + (!oldN_ * 2048) + (oldExposure_ * 64) + (cc & 4)
					: 129792 + (   !n_ * 2048) + (   exposure_ * 64) + (cc & 4);
				lastCycles_ = cc;
				bool success = false;
				if (cameraCallback_)
					success = cameraCallback_(cameraBuf_);

				if (!success)
					std::memset(cameraBuf_, 0x00, sizeof cameraBuf_);
			}
		}
		trigger_ = data & 0x07;
		break;
	}
	case 0x01:
		n_ = data & 0x80;
		vh_ = (data & 0x60) >> 5;
		break;
	case 0x02:
		exposure_ &= 0xFF;
		exposure_ |= (data << 8) & 0xFF00;
		break;
	case 0x03:
		exposure_ &= 0xFF00;
		exposure_ |= data & 0xFF;
		break;
	case 0x04:
	{
		unsigned edgeAlphaIndex = (data >> 4) & 0x07;
		edgeAlpha_ = (edgeAlphaIndex & 0x04) ? (edgeAlphaIndex - 2.0) : (0.50 + edgeAlphaIndex * 0.25);
		blank_ = data & 0x80;
		invert_ = data & 0x08;
		break;
	}
	case 0x05:
		// webcams handle this automatically, don't bother here
		break;
	default:
	{
		unsigned matrixIndex = (p & 0x7F) - 0x06;
		if (matrixIndex <= sizeof matrix_)
			matrix_[matrixIndex] = data & 0xFF;

		break;
	}
	}
}

void Camera::update(unsigned long const cc) {
	if (cameraCyclesLeft_ > 0) {
		cameraCyclesLeft_ -= (cc - lastCycles_) >> ds_;
		lastCycles_ = cc;
		if (cameraCyclesLeft_ <= 0) {
			trigger_ &= 0xFE;
			cancelled_ = false;
			if (cameraRam_)
				process();
		}
	}
}

void Camera::process() {
	unsigned char trigger;
	bool n;
	unsigned char vh;
	unsigned short exposure;
	float edgeAlpha;
	bool blank;
	bool invert;
	unsigned char matrix[4 * 4 * 3];

	if (cancelled_) {
		trigger = oldTrigger_;
		n = oldN_;
		vh = oldVh_;
		exposure = oldExposure_;
		edgeAlpha = oldEdgeAlpha_;
		blank = oldBlank_;
		invert = oldInvert_;
		std::memcpy(matrix, oldMatrix_, sizeof matrix);
	} else {
		trigger = trigger_;
		n = n_;
		vh = vh_;
		exposure = exposure_;
		edgeAlpha = edgeAlpha_;
		blank = blank_;
		invert = invert_;
		std::memcpy(matrix, matrix_, sizeof matrix);
	}

	for (unsigned i = 0; i < (128 * 112); i++) {
		int pixel = cameraBuf_[i];
		pixel = (pixel * exposure) / 0x0300;
		pixel = 0x80 + ((pixel - 0x80) / 0x08);
		cameraBuf_[i] = std::clamp(pixel, 0x00, 0xFF);
	}

	if (invert) {
		for (unsigned i = 0; i < (128 * 112); i++)
			cameraBuf_[i] = 0xFF - cameraBuf_[i];
	}

	for (unsigned i = 0; i < (128 * 112); i++)
		cameraBuf_[i] = cameraBuf_[i] - 0x80;

	int tempBuf[128 * 112];

	switch ((n << 3) | (vh << 1) | blank) {
	case 0x00: // 1-D filtering
		std::memcpy(tempBuf, cameraBuf_, sizeof tempBuf);
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = tempBuf[x + std::min(y + 1, 111) * 128];
			int px = tempBuf[i];

			int pixel = (trigger & 0x06) ? px : -px;
			if (trigger & 0x04)
				pixel -= ms;

			cameraBuf_[i] = std::clamp(pixel, -0x80, 0x7F);
		}
		break;
	case 0x01: // blank out???
		std::memset(cameraBuf_, 0x00, sizeof cameraBuf_);
		break;
	case 0x02: // 1-D filtering + Horizontal enhancement
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int mw = cameraBuf_[std::max(0, x - 1) + (y * 128)];
			int me = cameraBuf_[std::min(x + 1, 127) + (y * 128)];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)(px + ((2 * px - mw - me) * edgeAlpha)), 0x00, 0xFF);
		}
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = tempBuf[x + std::min(y + 1, 111) * 128];
			int px = tempBuf[i];

			int pixel = (trigger & 0x06) ? px : -px;
			if (trigger & 0x04)
				pixel -= ms;

			cameraBuf_[i] = std::clamp(pixel, -0x80, 0x7F);
		}
		break;
	case 0x03: // 1-D filtering + Horizontal extraction
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int mw = cameraBuf_[std::max(0, x - 1) + (y * 128)];
			int me = cameraBuf_[std::min(x + 1, 127) + (y * 128)];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)((2 * px - mw - me) * edgeAlpha), 0x00, 0xFF);
		}
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = tempBuf[x + std::min(y + 1, 111) * 128];
			int px = tempBuf[i];

			int pixel = (trigger & 0x06) ? px : -px;
			if (trigger & 0x04)
				pixel -= ms;

			cameraBuf_[i] = std::clamp(pixel, -0x80, 0x7F);
		}
		break;
	case 0x0C: // Vertical enhancement
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = cameraBuf_[x + std::min(y + 1, 111) * 128];
			int mn = cameraBuf_[x + std::max(0, y - 1) * 128];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)(px + ((2 * px - mn - ms) * edgeAlpha)), -0x80, 0x7F);
		}
		std::memcpy(cameraBuf_, tempBuf, sizeof cameraBuf_);
		break;
	case 0x0D: // Vertical extraction
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = cameraBuf_[x + std::min(y + 1, 111) * 128];
			int mn = cameraBuf_[x + std::max(0, y - 1) * 128];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)((2 * px - mn - ms) * edgeAlpha), -0x80, 0x7F);
		}
		std::memcpy(cameraBuf_, tempBuf, sizeof cameraBuf_);
		break;
	case 0x0E: // 2D enhancement
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = cameraBuf_[x + std::min(y + 1, 111) * 128];
			int mn = cameraBuf_[x + std::max(0, y - 1) * 128];
			int mw = cameraBuf_[std::max(0, x - 1) + (y * 128)];
			int me = cameraBuf_[std::min(x + 1, 127) + (y * 128)];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)(px + ((4 * px - mw - me - mn - ms) * edgeAlpha)), -0x80, 0x7F);
		}
		std::memcpy(cameraBuf_, tempBuf, sizeof cameraBuf_);
		break;
	case 0x0F: // 2D extraction
		for (unsigned i = 0; i < (128 * 112); i++) {
			int x = i % 128;
			int y = i / 128;
			int ms = cameraBuf_[x + std::min(y + 1, 111) * 128];
			int mn = cameraBuf_[x + std::max(0, y - 1) * 128];
			int mw = cameraBuf_[std::max(0, x - 1) + (y * 128)];
			int me = cameraBuf_[std::min(x + 1, 127) + (y * 128)];
			int px = cameraBuf_[i];

			tempBuf[i] = std::clamp((int)((4 * px - mw - me - mn - ms) * edgeAlpha), -0x80, 0x7F);
		}
		std::memcpy(cameraBuf_, tempBuf, sizeof cameraBuf_);
		break;
	default: // invalid filtering???
		break;
	}

	for (unsigned i = 0; i < (128 * 112); i++)
		cameraBuf_[i] = cameraBuf_[i] + 0x80;

	for (unsigned i = 0; i < (128 * 112); i++) {
		int x = (i % 128) & 3;
		int y = (i / 128) & 3;

		int base = (y * 4 + x) * 3;
		int color;

		if (cameraBuf_[i] < matrix[base])
			color = 0x00;
		else if (cameraBuf_[i] < matrix[base + 1])
			color = 0x01;
		else if (cameraBuf_[i] < matrix[base + 2])
			color = 0x02;
		else
			color = 0x03;

		tempBuf[i] = color;
	}

	unsigned char finalImage[14][16][16];
	std::memset(finalImage, 0x00, sizeof finalImage);

	for (unsigned i = 0; i < (128 * 112); i++) {
		int x = i % 128;
		int y = i / 128;

		unsigned char color = 3 - tempBuf[i];

		unsigned char *tile = finalImage[y >> 3][x >> 3];
		tile = &tile[(y & 7) * 2];

		if (color & 1)
			tile[0] |= 1 << (7 - (7 & x));
	
		if (color & 2)
			tile[1] |= 1 << (7 - (7 & x));
	}

	std::memcpy(cameraRam_, finalImage, sizeof finalImage);
}

SYNCFUNC(Camera) {
	NSS(cameraBuf_);
	NSS(trigger_);
	NSS(n_);
	NSS(vh_);
	NSS(exposure_);
	NSS(edgeAlpha_);
	NSS(blank_);
	NSS(invert_);
	NSS(matrix_);
	NSS(oldTrigger_);
	NSS(oldN_);
	NSS(oldVh_);
	NSS(oldExposure_);
	NSS(oldEdgeAlpha_);
	NSS(oldBlank_);
	NSS(oldInvert_);
	NSS(oldMatrix_);
	NSS(lastCycles_);
	NSS(cameraCyclesLeft_);
	NSS(cancelled_);
	NSS(ds_);
}

}
