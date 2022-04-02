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

#ifndef MBC_H
#define MBC_H

#include "../memptrs.h"
#include "../time.h"
#include "../rtc.h"
#include "../infrared.h"
#include "../huc3_chip.h"
#include "../camera.h"
#include "savestate.h"
#include "newstate.h"

#include <string>
#include <vector>

namespace gambatte {

inline unsigned rambanks(MemPtrs const &memptrs) {
	return (memptrs.rambankdataend() - memptrs.rambankdata()) / rambank_size();
}

inline unsigned rombanks(MemPtrs const &memptrs) {
	return (memptrs.romdataend() - memptrs.romdata()) / rombank_size();
}

class Mbc {
public:
	virtual ~Mbc() {}
	virtual bool disabledRam() const = 0;
	virtual void romWrite(unsigned p, unsigned data, unsigned long cycleCounter) = 0;
	virtual void saveState(SaveState::Mem &ss) const = 0;
	virtual void loadState(SaveState::Mem const &ss) = 0;
	virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned address, unsigned rombank) const = 0;
	template<bool isReader>void SyncState(NewState *ns) {
		// can't have virtual templates, so..
		SyncState(ns, isReader);
	}
	virtual void SyncState(NewState *ns, bool isReader) = 0;
};

class DefaultMbc : public Mbc {
public:
	virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const {
		return (addr < rombank_size()) == (bank == 0);
	}

	virtual void SyncState(NewState */*ns*/, bool /*isReader*/)
	{
	}
};

class Mbc0 : public DefaultMbc {
public:
	explicit Mbc0(MemPtrs &memptrs);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	bool enableRam_;
};

class Mbc1 : public Mbc {
public:
	explicit Mbc1(MemPtrs &memptrs, unsigned char rombankMask = 0x1Fu, unsigned char rombankShift = 0x05u);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const;
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	bool enableRam_;
	bool rambankMode_;
	unsigned char bankReg1_;
	unsigned char bankReg2_;
	unsigned char rombank0_;
	unsigned char rombank_;
	unsigned char rambank_;
	unsigned char rombankMask_;
	unsigned char rombankShift_;

	void updateBanking();
	void setRambank() const;
	void setRombank() const;
};

class Mbc1Multi64 : public Mbc1 {
public:
	explicit Mbc1Multi64(MemPtrs &memptrs)
	: Mbc1(memptrs, 0x0Fu, 0x04u)
	{
	}
};

class Mbc2 : public DefaultMbc {
public:
	explicit Mbc2(MemPtrs &memptrs);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	unsigned char rombank_;
	bool enableRam_;
};

class Mbc3 : public DefaultMbc {
public:
	Mbc3(MemPtrs &memptrs, Rtc *const rtc, unsigned char rombankMask = 0x7Fu, unsigned char rambankMask = 0x03u);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const cc);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	Rtc *const rtc_;
	unsigned char rombank_;
	unsigned char rambank_;
	bool enableRam_;
	unsigned char rombankMask_;
	unsigned char rambankMask_;

	void setRambank(unsigned flags = MemPtrs::read_en | MemPtrs::write_en) const;
	void setRombank() const;
};

class Mbc30 : public Mbc3 {
public:
	Mbc30(MemPtrs &memptrs, Rtc *const rtc)
	: Mbc3(memptrs, rtc, 0xFFu, 0x07u)
	{
	}
};

class Mbc5 : public DefaultMbc {
public:
	explicit Mbc5(MemPtrs &memptrs);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	unsigned short rombank_;
	unsigned char rambank_;
	bool enableRam_;

	void setRambank() const;
	void setRombank() const;
};

class Mmm01 : public Mbc {
public:
	explicit Mmm01(MemPtrs &memptrs);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const;
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	unsigned char bankReg0_;
	unsigned char bankReg1_;
	unsigned char bankReg2_;
	unsigned char bankReg3_;

	bool isMapped() const;
	bool enableRam() const;
	unsigned char rombankLow(bool upper) const;
	unsigned char rombankMid() const;
	unsigned char rombankHigh() const;
	unsigned char rombankMask() const;
	unsigned char rambankLow(bool noMode) const;
	unsigned char rambankHigh() const;
	unsigned char rambankMask() const;
	bool isMuxed() const;
	void updateBanking() const;
};

class HuC1 : public DefaultMbc {
public:
	explicit HuC1(MemPtrs &memptrs, Infrared *const ir_);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	Infrared *const ir_;
	unsigned char rombank_;
	unsigned char rambank_;
	unsigned char ramflag_;

	void setRambank() const;
	void setRombank() const;
};

class HuC3 : public DefaultMbc {
public:
	explicit HuC3(MemPtrs &memptrs, HuC3Chip *const huc3);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	HuC3Chip *const huc3_;
	unsigned char rombank_;
	unsigned char rambank_;
	unsigned char ramflag_;

	void setRambank(bool setRamflag = true) const;
	void setRombank() const;
};

class PocketCamera : public DefaultMbc {
public:
	explicit PocketCamera(MemPtrs &memptrs, Camera *const camera);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const data, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const &ss);
	virtual void SyncState(NewState *ns, bool isReader);

private:
	MemPtrs &memptrs_;
	Camera *const camera_;
	unsigned char rombank_;
	unsigned char rambank_;
	bool enableRam_;

	void setRambank() const;
	void setRombank() const;
};

class WisdomTree : public Mbc {
public:
	explicit WisdomTree(MemPtrs& memptrs);
	virtual bool disabledRam() const;
	virtual void romWrite(unsigned const p, unsigned const /*data*/, unsigned long const /*cc*/);
	virtual void saveState(SaveState::Mem &ss) const;
	virtual void loadState(SaveState::Mem const& ss);
	virtual bool isAddressWithinAreaRombankCanBeMappedTo(unsigned addr, unsigned bank) const;
	virtual void SyncState(NewState* ns, bool isReader);

private:
	MemPtrs& memptrs_;
	unsigned char rombank_;

	void setRombank() const;
};

}

#endif
