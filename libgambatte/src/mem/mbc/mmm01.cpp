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

#include "mbc.h"

namespace gambatte {

Mmm01::Mmm01(MemPtrs &memptrs)
: memptrs_(memptrs)
, bankReg0_(0)
, bankReg1_(1)
, bankReg2_(0)
, bankReg3_(0)
{
	updateBanking();
}

bool Mmm01::disabledRam() const {
	return !enableRam();
}

void Mmm01::romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/) {
	switch (p >> 13 & 3) {
	case 0:
		if (isMapped())
			bankReg0_ = (bankReg0_ & 0x70) | (data & 0x0F);
		else
			bankReg0_ = data & 0x7F;

		updateBanking();
		break;
	case 1:
		if (!isMapped())
			bankReg1_ = (bankReg1_ & 0x1F) | (data & 0x60);

		bankReg1_ &= rombankMask() | 0x60;
		bankReg1_ |= ~rombankMask() & data & 0x1F;
		bankReg1_ = (bankReg1_ & 0x7E) | (data & 0x01); // bit 0 is always writable!
		updateBanking();
		break;
	case 2:
		if (!isMapped())
			bankReg2_ = (bankReg2_ & 0x03) | (data & 0x7C);

		bankReg2_ &= rambankMask() | 0x7C;
		bankReg2_ |= ~rambankMask() & data & 3;
		updateBanking();
		break;
	case 3:
		if (!isMapped())
			bankReg3_ = (bankReg3_ & 0x01) | (data & 0x7E);

		if (!(bankReg2_ & 0x40))
			bankReg3_ = (bankReg3_ & 0x7E) | (data & 0x01);

		updateBanking();
		break;
	}
}

// fixme: this is kinda hacky but it works i guess
void Mmm01::saveState(SaveState::Mem &ss) const {
	ss.ramflag = bankReg0_;
	ss.rombank = bankReg1_;
	ss.rambank = bankReg2_;
	ss.rambankMode = bankReg3_;
}

void Mmm01::loadState(SaveState::Mem const &ss) {
	bankReg0_ = ss.ramflag;
	bankReg1_ = ss.rombank;
	bankReg2_ = ss.rambank;
	bankReg3_ = ss.rambankMode;
	updateBanking();
}

void Mmm01::SyncState(NewState *ns, bool isReader) {
	NSS(bankReg0_);
	NSS(bankReg1_);
	NSS(bankReg2_);
	NSS(bankReg3_);
}

bool Mmm01::isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
	if (isMapped())
		return (addr < 0x4000) == ((bank & (bankReg3_ >> 2 & 0x0F)) == 0); // ???
	else
		return addr < 0x4000 ? bank == 0x1FE : bank == 0x1FF;
}

inline bool Mmm01::isMapped() const {
	return bankReg0_ & 0x40;
}

inline bool Mmm01::enableRam() const {
	return (bankReg0_ & 0xF) == 0xA;
}

inline unsigned char Mmm01::rombankLow(bool upper) const {
	unsigned char bank = (bankReg1_ & 0x1F) & rombankMask();
	if (upper)
		bank |= std::max((bankReg1_ & 0x1F) & ~rombankMask(), 1);

	return bank;
}

inline unsigned char Mmm01::rombankMid() const {
	return bankReg1_ >> 5 & 3; // muxable
}

inline unsigned char Mmm01::rombankHigh() const {
	return bankReg2_ >> 4 & 3;
}

inline unsigned char Mmm01::rombankMask() const {
	return bankReg3_ >> 1 & 0x1E;
}

inline unsigned char Mmm01::rambankLow(bool noMode) const {
	return (bankReg2_ & 3) * ((bankReg3_ & 1) | noMode); // muxable
}

inline unsigned char Mmm01::rambankHigh() const {
	return bankReg2_ >> 2 & 3;
}

inline unsigned char Mmm01::rambankMask() const {
	return bankReg0_ >> 4 & 3;
}

inline bool Mmm01::isMuxed() const {
	return bankReg3_ & 0x40;
}

void Mmm01::updateBanking() const {
	unsigned rombank0 = 0x1FE;
	unsigned rombank = 0x1FF;

	if (isMapped()) {
		rombank0 = rombankLow(false) | (isMuxed() ? rambankLow(false) : rombankMid()) << 5 | rombankHigh() << 7;
		rombank = rombankLow(true) | (isMuxed() ? rambankLow(true) : rombankMid()) << 5 | rombankHigh() << 7;
	}

	unsigned rambank = (isMuxed() ? rombankMid() : rambankLow(false)) | rambankHigh() << 2;

	memptrs_.setRombank0(rombank0 & (rombanks(memptrs_) - 1));
	memptrs_.setRombank(rombank & (rombanks(memptrs_) - 1));
	memptrs_.setRambank(enableRam() ? MemPtrs::read_en | MemPtrs::write_en : MemPtrs::disabled,
	                    rambank & (rambanks(memptrs_) - 1));
}

}
