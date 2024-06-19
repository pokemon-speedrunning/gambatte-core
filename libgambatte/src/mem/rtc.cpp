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

#include "rtc.h"
#include "../savestate.h"

namespace gambatte {

Rtc::Rtc(Time &time)
: time_(time)
, activeLatch_(0)
, activeSet_(0)
, index_(5)
, enabled_(false)
, dataDh_(0)
, dataDl_(0)
, dataH_(0)
, dataM_(0)
, dataS_(0)
, dataC_(0)
, latchDh_(0)
, latchDl_(0)
, latchH_(0)
, latchM_(0)
, latchS_(0)
{
}

void Rtc::updateClock(unsigned long const cc) {
	unsigned long const cycleDivisor = time_.getRtcDivisor();
	unsigned long long diff = time_.diff(cc);
	if (!(dataDh_ & 0x40)) {
		dataC_ += diff % cycleDivisor;
		if (dataC_ >= cycleDivisor) {
			dataS_++;
			dataC_ -= cycleDivisor;
		}
		diff /= cycleDivisor;
		dataS_ += diff % 60;
		if (dataS_ >= 60) {
			dataM_++;
			dataS_ -= 60;
		}
		diff /= 60;
		dataM_ += diff % 60;
		if (dataM_ >= 60) {
			dataH_++;
			dataM_ -= 60;
		}
		diff /= 60;
		dataH_ += diff % 24;
		unsigned long days = ((dataDh_ & 0x01) << 8) | dataDl_;
		if (dataH_ >= 24) {
			days++;
			dataH_ -= 24;
		}
		days += diff / 24;
		dataDl_ = days & 0xFF;
		dataDh_ &= 0xFE;
		dataDh_ |= (days >> 8) & 0x01;
		if (days >> 9)
			dataDh_ |= 0x80;
	}
}

void Rtc::setTime(unsigned long long const dividers) {
	unsigned long const cycleDivisor = time_.getRtcDivisor();
	dataC_ = dividers * 2 % cycleDivisor;
	dataS_ = dividers * 2 / cycleDivisor % 60;
	dataM_ = dividers * 2 / cycleDivisor / 60 % 60;
	dataH_ = dividers * 2 / cycleDivisor / 60 / 60 % 24;
	unsigned long const days = dividers * 2 / cycleDivisor / 60 / 60 / 24;
	dataDl_ = days & 0xFF;
	dataDh_ &= 0xFE;
	dataDh_ |= (days >> 8) & 0x01;
	dataDh_ &= ~0x40u;
	if (days >> 9)
		dataDh_ |= 0x80;
}

void Rtc::doLatch(unsigned long const cc) {
	updateClock(cc);
	latchDh_ = dataDh_;
	latchDl_ = dataDl_;
	latchH_ = dataH_;
	if (dataH_ < 0)
		latchH_ += 0x20;

	latchM_ = dataM_;
	if (dataM_ < 0)
		latchM_ += 0x40;

	latchS_ = dataS_;
	if (dataS_ < 0)
		latchS_ += 0x40;
}

void Rtc::doSwapActive() {
	if (!enabled_ || index_ > 4) {
		activeLatch_ = 0;
		activeSet_ = 0;
	} else switch (index_) {
	case 0x00:
		activeLatch_ = &latchS_;
		activeSet_ = &Rtc::setS;
		break;
	case 0x01:
		activeLatch_ = &latchM_;
		activeSet_ = &Rtc::setM;
		break;
	case 0x02:
		activeLatch_ = &latchH_;
		activeSet_ = &Rtc::setH;
		break;
	case 0x03:
		activeLatch_ = &latchDl_;
		activeSet_ = &Rtc::setDl;
		break;
	case 0x04:
		activeLatch_ = &latchDh_;
		activeSet_ = &Rtc::setDh;
		break;
	}
}

void Rtc::saveState(SaveState &state) const {
	state.rtc.dataDh = dataDh_;
	state.rtc.dataDl = dataDl_;
	state.rtc.dataH = dataH_;
	if (dataH_ < 0)
		state.rtc.dataH += 0x20;
	
	state.rtc.dataM = dataM_;
	if (dataM_ < 0)
		state.rtc.dataM += 0x40;

	state.rtc.dataS = dataS_;
	if (dataS_ < 0)
		state.rtc.dataS += 0x40;

	state.rtc.dataC = dataC_;
	state.rtc.latchDh = latchDh_;
	state.rtc.latchDl = latchDl_;
	state.rtc.latchH = latchH_;
	state.rtc.latchM = latchM_;
	state.rtc.latchS = latchS_;
}

void Rtc::loadState(SaveState const &state) {
	dataDh_ = state.rtc.dataDh;
	dataDl_ = state.rtc.dataDl;
	dataH_ = state.rtc.dataH;
	if (dataH_ >= 24)
		dataH_ -= 0x20;

	dataM_ = state.rtc.dataM;
	if (dataM_ >= 60)
		dataM_ -= 0x40;

	dataS_ = state.rtc.dataS;
	if (dataS_ >= 60)
		dataS_ -= 0x40;

	dataC_ = state.rtc.dataC;
	latchDh_ = state.rtc.latchDh;
	latchDl_ = state.rtc.latchDl;
	latchH_ = state.rtc.latchH;
	latchM_ = state.rtc.latchM;
	latchS_ = state.rtc.latchS;
	doSwapActive();
}

void Rtc::getRtcRegs(unsigned long *dest, unsigned long const cc) {
	updateClock(cc);
	dest[rtc_dh] = dataDh_;
	dest[rtc_dl] = dataDl_;
	dest[rtc_h] = dataH_;
	if (dataH_ < 0)
		dest[rtc_h] += 0x20;

	dest[rtc_m] = dataM_;
	if (dataM_ < 0)
		dest[rtc_m] += 0x40;

	dest[rtc_s] = dataS_;
	if (dataS_ < 0)
		dest[rtc_s] += 0x40;

	dest[rtc_c] = dataC_;
	dest[rtc_dh + rtc_l] = latchDh_;
	dest[rtc_dl + rtc_l] = latchDl_;
	dest[rtc_h + rtc_l] = latchH_;
	dest[rtc_m + rtc_l] = latchM_;
	dest[rtc_s + rtc_l] = latchS_;
}

void Rtc::setRtcRegs(unsigned long *src) {
	dataDh_ = src[rtc_dh];
	dataDl_ = src[rtc_dl];
	dataH_ = src[rtc_h];
	if (dataH_ >= 24)
		dataH_ -= 0x20;

	dataM_ = src[rtc_m];
	if (dataM_ >= 60)
		dataM_ -= 0x40;

	dataS_ = src[rtc_s];
	if (dataS_ >= 60)
		dataS_ -= 0x40;

	dataC_ = src[rtc_c];
	latchDh_ = src[rtc_dh + rtc_l];
	latchDl_ = src[rtc_dl + rtc_l];
	latchH_ = src[rtc_h + rtc_l];
	latchM_ = src[rtc_m + rtc_l];
	latchS_ = src[rtc_s + rtc_l];
}

void Rtc::setBaseTime(unsigned long long baseTime, unsigned long const cc) {
	unsigned long const cycleDivisor = time_.getRtcDivisor();
	unsigned long long diff = (std::time(0) - baseTime) * cycleDivisor + cc;
	if (!(dataDh_ & 0x40)) {
		dataC_ += diff % cycleDivisor;
		if (dataC_ >= cycleDivisor) {
			dataS_++;
			dataC_ -= cycleDivisor;
		}
		diff /= cycleDivisor;
		dataS_ += diff % 60;
		if (dataS_ >= 60) {
			dataM_++;
			dataS_ -= 60;
		}
		diff /= 60;
		dataM_ += diff % 60;
		if (dataM_ >= 60) {
			dataH_++;
			dataM_ -= 60;
		}
		diff /= 60;
		dataH_ += diff % 24;
		unsigned long days = ((dataDh_ & 0x01) << 8) | dataDl_;
		if (dataH_ >= 24) {
			days++;
			dataH_ -= 24;
		}
		days += diff / 24;
		dataDl_ = days & 0xFF;
		dataDh_ &= 0xFE;
		dataDh_ |= (days >> 8) & 0x01;
		if (days >> 9)
			dataDh_ |= 0x80;
	}
}

void Rtc::setDh(unsigned const newDh, unsigned const long cc) {
	updateClock(cc);
	dataDh_ = newDh & 0xC1;
}

void Rtc::setDl(unsigned const newLowdays, unsigned const long cc) {
	updateClock(cc);
	dataDl_ = newLowdays;
}

void Rtc::setH(unsigned const newHours, unsigned const long cc) {
	updateClock(cc);
	dataH_ = newHours & 0x1F;
	if (dataH_ >= 24)
		dataH_ -= 0x20;
}

void Rtc::setM(unsigned const newMinutes, unsigned const long cc) {
	updateClock(cc);
	dataM_ = newMinutes & 0x3F;
	if (dataM_ >= 60)
		dataM_ -= 0x40;
}

void Rtc::setS(unsigned const newSeconds, unsigned const long cc) {
	updateClock(cc);
	dataS_ = newSeconds & 0x3F;
	if (dataS_ >= 60)
		dataS_ -= 0x40;

	dataC_ = 0;
}

SYNCFUNC(Rtc) {
	EBS(activeLatch_, 0);
	EVS(activeLatch_, &latchS_, 1);
	EVS(activeLatch_, &latchM_, 2);
	EVS(activeLatch_, &latchH_, 3);
	EVS(activeLatch_, &latchDl_, 4);
	EVS(activeLatch_, &latchDh_, 5);
	EES(activeLatch_, NULL);

	EBS(activeSet_, 0);
	EVS(activeSet_, &Rtc::setS, 1);
	EVS(activeSet_, &Rtc::setM, 2);
	EVS(activeSet_, &Rtc::setH, 3);
	EVS(activeSet_, &Rtc::setDl, 4);
	EVS(activeSet_, &Rtc::setDh, 5);
	EES(activeSet_, NULL);

	NSS(index_);
	NSS(enabled_);
	NSS(dataDh_);
	NSS(dataDl_);
	NSS(dataH_);
	NSS(dataM_);
	NSS(dataS_);
	NSS(dataC_);
	NSS(latchDh_);
	NSS(latchDl_);
	NSS(latchH_);
	NSS(latchM_);
	NSS(latchS_);
}

}
