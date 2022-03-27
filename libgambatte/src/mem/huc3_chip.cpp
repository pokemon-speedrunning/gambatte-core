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

#include "huc3_chip.h"
#include "../savestate.h"

#include <algorithm>

namespace gambatte {

HuC3Chip::HuC3Chip(Time &time)
: time_(time)
, ioIndex_(0)
, transferValue_(0)
, ramflag_(0)
, irBaseCycle_(0)
, rtcCycles_(0)
, currentSample_(0)
, toneBufPos_(0)
, toneLastUpdate_(0)
, nextPhaseChangeTime_(0)
, remainingToneSamples_(0)
, enabled_(false)
, committing_(false)
, highIoReadOnly_(true)
, irReceivingPulse_(false)
, ds_(false)
{
	std::memset(io_, 0, sizeof io_);
	std::memset(toneBuf_, 0, sizeof toneBuf_);
}

void HuC3Chip::updateClock(unsigned long const cc) {
	unsigned long const cycleDivisor = time_.getRtcDivisor() * 60;
	unsigned long diff = time_.diff(cc);
	if (io_[0x16] & 1) {
		unsigned long minutes = (io_[0x10] & 0x0F) | ((io_[0x11] & 0x0F) << 4) | ((io_[0x12] & 0x0F) << 8);
		unsigned long days = (io_[0x13] & 0x0F) | ((io_[0x14] & 0x0F) << 4) | ((io_[0x15] & 0x0F) << 8);
		rtcCycles_ += diff % cycleDivisor;
		if (rtcCycles_ >= cycleDivisor) {
			minutes++;
			rtcCycles_ -= cycleDivisor;
		}
		diff /= cycleDivisor;
		minutes += diff % 1440;
		if (minutes >= 1440) {
			days++;
			minutes -= 1440;
		}
		diff /= 1440;
		days += diff;
		io_[0x10] = minutes & 0x0F;
		io_[0x11] = (minutes >> 4) & 0x0F;
		io_[0x12] = (minutes >> 8) & 0x0F;
		io_[0x13] = days & 0x0F;
		io_[0x14] = (days >> 4) & 0x0F;
		io_[0x15] = (days >> 8) & 0x0F;
	}
}

unsigned HuC3Chip::timeNow() const {
	unsigned long const minutes = (io_[0x10] & 0x0F) | ((io_[0x11] & 0x0F) << 4) | ((io_[0x12] & 0x0F) << 8);
	unsigned long const days = (io_[0x13] & 0x0F) | ((io_[0x14] & 0x0F) << 4) | ((io_[0x15] & 0x0F) << 8);
	return ((days * 86400 + minutes * 60) * time_.getRtcDivisor() + rtcCycles_) >> 1;
}

void HuC3Chip::setBaseTime(timeval baseTime, unsigned long const cc) {
	unsigned long const cycleDivisor = time_.getRtcDivisor() * 60;
	timeval now_ = Time::now();
	unsigned long long diff = ((now_.tv_sec - baseTime.tv_sec) * cycleDivisor / 60)
	+ (((now_.tv_usec - baseTime.tv_usec) * cycleDivisor) / 1000000.0f)
	+ cc;
	if (io_[0x16] & 1) {
		unsigned long minutes = (io_[0x10] & 0x0F) | ((io_[0x11] & 0x0F) << 4) | ((io_[0x12] & 0x0F) << 8);
		unsigned long days = (io_[0x13] & 0x0F) | ((io_[0x14] & 0x0F) << 4) | ((io_[0x15] & 0x0F) << 8);
		rtcCycles_ += diff % cycleDivisor;
		if (rtcCycles_ >= cycleDivisor) {
			minutes++;
			rtcCycles_ -= cycleDivisor;
		}
		diff /= cycleDivisor;
		minutes += diff % 1440;
		if (minutes >= 1440) {
			days++;
			minutes -= 1440;
		}
		diff /= 1440;
		days += diff;
		io_[0x10] = minutes & 0x0F;
		io_[0x11] = (minutes >> 4) & 0x0F;
		io_[0x12] = (minutes >> 8) & 0x0F;
		io_[0x13] = days & 0x0F;
		io_[0x14] = (days >> 4) & 0x0F;
		io_[0x15] = (days >> 8) & 0x0F;
	}
}

void HuC3Chip::getHuC3Regs(unsigned char *dest, unsigned long const cc) {
	updateClock(cc);
	dest[0] = (rtcCycles_ >> 24) & 0xFF;
	dest[1] = (rtcCycles_ >> 16) & 0xFF;
	dest[2] = (rtcCycles_ >> 8) & 0xFF;
	dest[3] = rtcCycles_ & 0xFF;
	std::memcpy(&dest[4], io_, sizeof io_);
}

void HuC3Chip::setHuC3Regs(unsigned char *src) {
	rtcCycles_ = src[0] & 0xFF;
	rtcCycles_ = (rtcCycles_ << 8) | (src[1] & 0xFF);
	rtcCycles_ = (rtcCycles_ << 8) | (src[2] & 0xFF);
	rtcCycles_ = (rtcCycles_ << 8) | (src[3] & 0xFF);
	std::memcpy(io_, &src[4], sizeof io_);
}

// fixme: this is assuming a "BING BONG" sound
void HuC3Chip::accumulateSamples(unsigned long const cc) {
	unsigned long samples = (cc - toneLastUpdate_) >> (1 + ds_);
	toneLastUpdate_ = cc;
	if ((toneBufPos_ + samples) >= max_samples)
		samples = max_samples - toneBufPos_ - 1;

	while (remainingToneSamples_ > 0 && samples > 0) {
		unsigned samplesToWrite = std::min(samples, nextPhaseChangeTime_);
		remainingToneSamples_ -= samplesToWrite;
		if (remainingToneSamples_ < 0) {
			samplesToWrite += remainingToneSamples_;
			remainingToneSamples_ = 0;
		}
		std::fill_n(&toneBuf_[toneBufPos_ << 1], samplesToWrite << 1, currentSample_ * ((io_[0x72] & 0x8) >> 2));
		toneBufPos_ += samplesToWrite;
		samples -= samplesToWrite;
		nextPhaseChangeTime_ -= samplesToWrite;
		if (nextPhaseChangeTime_ == 0) {
			if (remainingToneSamples_ > 2097152)
				nextPhaseChangeTime_ = 2097; // ~1000 hz
			else
				nextPhaseChangeTime_ = 2796; // ~750 hz

			if ((3145728 - remainingToneSamples_) == 501 * 2097)
				currentSample_ = -0x6000;

			currentSample_ *= remainingToneSamples_ > 2097152 ? 0.997 : 0.995;
			currentSample_ = -currentSample_;
		}
	}

	if (samples) {
		std::fill_n(&toneBuf_[toneBufPos_ << 1], (samples << 1), 0);
		toneBufPos_ += samples;
	}
}

unsigned HuC3Chip::generateSamples(short *soundBuf) {
	if (!enabled_)
		return 0;

	unsigned samples = toneBufPos_;
	std::memcpy(soundBuf, toneBuf_, (samples << 1) * sizeof (short));
	toneBufPos_ = 0;
	return samples;
}

void HuC3Chip::resetCc(unsigned long const oldCc, unsigned long const newCc) {
	accumulateSamples(oldCc);
	toneLastUpdate_ -= oldCc - newCc;
}

void HuC3Chip::speedChange(unsigned long const cc) {
	accumulateSamples(cc);
	ds_ = !ds_;
}

void HuC3Chip::setStatePtrs(SaveState &state) {
	state.huc3.io.set(io_, sizeof io_);
}

void HuC3Chip::saveState(SaveState &state) const {
	state.huc3.ioIndex = ioIndex_;
	state.huc3.transferValue = transferValue_;
	state.huc3.ramflag = ramflag_;
	state.huc3.irBaseCycle = irBaseCycle_;
	state.huc3.rtcCycles = rtcCycles_;
	state.huc3.currentSample = currentSample_;
	state.huc3.toneLastUpdate = toneLastUpdate_;
	state.huc3.nextPhaseChangeTime = nextPhaseChangeTime_;
	state.huc3.remainingToneSamples = remainingToneSamples_;
	state.huc3.committing = committing_;
	state.huc3.highIoReadOnly = highIoReadOnly_;
	state.huc3.irReceivingPulse = irReceivingPulse_;
}

void HuC3Chip::loadState(SaveState const &state, bool cgb) {
	ioIndex_ = state.huc3.ioIndex;
	transferValue_ = state.huc3.transferValue;
	ramflag_ = state.huc3.ramflag;
	irBaseCycle_ = state.huc3.irBaseCycle;
	rtcCycles_ = state.huc3.rtcCycles;
	currentSample_ = state.huc3.currentSample;
	toneLastUpdate_ = state.huc3.toneLastUpdate;
	nextPhaseChangeTime_ = state.huc3.nextPhaseChangeTime;
	remainingToneSamples_ = state.huc3.remainingToneSamples;
	committing_ = state.huc3.committing;
	highIoReadOnly_ = state.huc3.highIoReadOnly;
	irReceivingPulse_ = state.huc3.irReceivingPulse;
	ds_ = (cgb && state.ppu.notCgbDmg) & state.mem.ioamhram.get()[0x14D] >> 7;
}

unsigned char HuC3Chip::read(unsigned /*p*/, unsigned long const cc) {
	// should only reach here with ramflag = 0B-0E
	switch (ramflag_) {
		case 0xB: // write mode
		case 0xC: // read mode
			return 0x80 | transferValue_;
		case 0xD: // commit mode
			return 0xFE | committing_;
		case 0xE: // IR mode
		{
			if (!irReceivingPulse_) {
				irReceivingPulse_ = true;
				irBaseCycle_ = cc;
			}
			unsigned long cyclesSinceStart = cc - irBaseCycle_;
			unsigned char modulation = (cyclesSinceStart / 105) & 1; // 4194304 Hz CPU, 40000 Hz remote signal
			unsigned long timeUs = cyclesSinceStart * 36 / 151; // actually *1000000/4194304
			// sony protocol
			if (timeUs < 10000) // initialization allowance
				return 0;
			else if (timeUs < 10000 + 2400) // initial mark
				return modulation;
			else if (timeUs < 10000 + 2400 + 600) // initial space
				return 0;
			else { // send data
				timeUs -= 13000;
				unsigned int data = 0xFFFFF; // write 20 bits (any 20 seem to do)
				for (unsigned long mask = 1ul << (20 - 1); mask; mask >>= 1) {
					unsigned int markTime = (data & mask) ? 1200 : 600;
					if (timeUs < markTime)
						return modulation;

					timeUs -= markTime;
					if (timeUs < 600)
						return 0;

					timeUs -= 600;
				}

				return 0;
			}
		}
	}

	return 0xFF;
}

void HuC3Chip::write(unsigned /*p*/, unsigned data, unsigned long const cc) {
	switch (ramflag_) {
		case 0xB: // write mode
			transferValue_ = 0x80 | data;
			break;
		case 0xC: // read mode
			break;
		case 0xD: // commit mode
			if (committing_ && !(data & 1)) {
				switch (transferValue_ & 0x70) {
					case 0x10:
						updateClock(cc);
						accumulateSamples(cc);
						transferValue_ = (io_[ioIndex_] & 0x0F) | (transferValue_ & 0xF0);
						ioIndex_ = (ioIndex_ + 1) & 0xFF;
						break;
					case 0x30:
						updateClock(cc);
						accumulateSamples(cc);
						if (ioIndex_ < 0x20 || !highIoReadOnly_)
							io_[ioIndex_] = transferValue_ & 0x0F;

						ioIndex_ = (ioIndex_ + 1) & 0xFF;
						break;
					case 0x40:
						ioIndex_ = (transferValue_ & 0x0F) | (ioIndex_ & 0xF0);
						break;
					case 0x50:
						ioIndex_ = ((transferValue_ & 0x0F) << 4) | (ioIndex_ & 0x0F);
						break;
					case 0x60:
						switch (transferValue_ & 0xF) {
							case 0x0: // latch rtc
								updateClock(cc);
								std::memcpy(&io_[0x00], &io_[0x10], 0x07);
								highIoReadOnly_ = false;
								break;
							case 0x1: // set rtc
								updateClock(cc);
								std::memcpy(&io_[0x10], &io_[0x00], 0x07);
								highIoReadOnly_ = false;
								break;
							case 0x2: // set high io (0x20-0xFF) to read-only
								highIoReadOnly_ = true;
								break;
							case 0xE: // tone
								accumulateSamples(cc);
								if ((io_[0x27] & 0xF) == 1) {
									if (remainingToneSamples_ >= 0)
										remainingToneSamples_ = -1;
									else {
										remainingToneSamples_ = 2097152 * 3 / 2; // 1.5 seconds of tone
										nextPhaseChangeTime_ = 2097; // ~1000 hz
										currentSample_ = 0x6000;
									}
								}
								// fallthrough
							default:
								highIoReadOnly_ = false;
								break;
						}
						transferValue_ = 0x80 | 0x61;
						break;
				}

				committing_ = false;
			}
	}
}

SYNCFUNC(HuC3Chip) {
	NSS(io_);
	NSS(ioIndex_);
	NSS(transferValue_);
	NSS(ramflag_);
	NSS(irBaseCycle_);
	NSS(rtcCycles_);
	NSS(currentSample_);
	NSS(toneLastUpdate_);
	NSS(nextPhaseChangeTime_);
	NSS(remainingToneSamples_);
	NSS(enabled_);
	NSS(committing_);
	NSS(highIoReadOnly_);
	NSS(irReceivingPulse_);
}

}
