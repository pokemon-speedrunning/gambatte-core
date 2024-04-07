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

#ifndef MEMORY_H
#define MEMORY_H

#include "mem/cartridge.h"
#include "mem/sgb.h"
#include "inputgetter.h"
#include "interrupter.h"
#include "pakinfo.h"
#include "sound.h"
#include "tima.h"
#include "video.h"
#include "gambatte.h"
#include "newstate.h"

#include <cstring>

namespace gambatte {

class FilterInfo;

class Memory {
public:
	explicit Memory(Interrupter const &interrupter);
	bool loaded() const { return cart_.loaded(); }
	unsigned getBank(unsigned type) { return cart_.getBank(type); }
	unsigned getAddrBank(unsigned short addr) { return cart_.getAddrBank(addr); }
	void setBank(unsigned type, unsigned bank) { cart_.setBank(type, bank); }
	void setAddrBank(unsigned short addr, unsigned bank) { cart_.setAddrBank(addr, bank); }
	char const * romTitle() const { return cart_.romTitle(); }
	PakInfo const pakInfo() const { return cart_.pakInfo(); }
	void setStatePtrs(SaveState &state);
	unsigned long saveState(SaveState &state, unsigned long cc);
	void loadState(SaveState const &state);
	void loadSavedata(unsigned long const cc) { cart_.loadSavedata(cc); }
	void saveSavedata(unsigned long const cc) { cart_.saveSavedata(cc); }
	unsigned getSavedataLength() { return cart_.getSavedataLength(); }
	void loadSavedata(char const *data, unsigned long const cc) { cart_.loadSavedata(data, cc); }
	void saveSavedata(char *dest, unsigned long const cc) { cart_.saveSavedata(dest, cc); }

	std::string const saveBasePath() const { return cart_.saveBasePath(); }

	void setOsdElement(transfer_ptr<OsdElement> osdElement) {
		lcd_.setOsdElement(osdElement);
	}

	bool getMemoryArea(int which, unsigned char **data, int *length);

	unsigned long stop(unsigned long cycleCounter, bool &prefetched);
	void stall(unsigned long cycleCounter, unsigned long cycles);
	bool isCgb() const { return lcd_.isCgb(); }
	bool isCgbDmg() const { return lcd_.isCgbDmg(); }
	bool isSgb() const { return gbIsSgb_; }
	bool ime() const { return intreq_.ime(); }
	bool halted() const { return intreq_.halted(); }
	unsigned long nextEventTime() const { return intreq_.minEventTime(); }
	void setLayers(unsigned mask) { lcd_.setLayers(mask); }
	bool isActive() const { return intreq_.eventTime(intevent_end) != disabled_time; }

	long cyclesSinceBlit(unsigned long cc) const {
		if (cc < intreq_.eventTime(intevent_blit))
			return -1;

		return (cc - intreq_.eventTime(intevent_blit)) >> isDoubleSpeed();
	}

	void freeze(unsigned long cc);
	bool halt(unsigned long cc);
	void ei(unsigned long cycleCounter) { if (!ime()) { intreq_.ei(cycleCounter); } }
	void di() { intreq_.di(); }
	unsigned pendingIrqs(unsigned long cc);
	void ackIrq(unsigned bit, unsigned long cc);

	unsigned readBios(unsigned p) {
		return bios_.get()[p];
	}

	template <bool callbacksActive>
	unsigned ff_read(unsigned p, unsigned long cc) {
		if (callbacksActive && readCallback_)
			readCallback_(p, callbackCycleOffset(cc), 0);

		return p < 0x80 ? nontrivial_ff_read(p, cc) : ioamhram_[p + 0x100];
	}

	struct CDMapResult {
		eCDLog_AddrType type;
		unsigned addr;
	};

	CDMapResult CDMap(const unsigned p) const {
		if (p < mm_rom1_begin) {
			CDMapResult ret = { eCDLog_AddrType_ROM, p };
			return ret;
		} else if (p < mm_vram_begin) {
			unsigned bank = cart_.rmem(p >> 12) - cart_.rmem(0);
			unsigned addr = p + bank;
			CDMapResult ret = { eCDLog_AddrType_ROM, addr };
			return ret;
		} else if (p < mm_sram_begin) {
		} else if (p < mm_wram_begin) {
			if (cart_.wsrambankptr()) {
				//not bankable. but. we're not sure how much might be here
				unsigned char *data;
				int length;
				bool has = cart_.getMemoryArea(3, &data, &length);
				unsigned addr = p & (length - 1);
				if (has && length) {
					CDMapResult ret = { eCDLog_AddrType_CartRAM, addr };
					return ret;
				}
			}
		} else if (p < mm_wram_mirror_begin) {
			unsigned bank = cart_.wramdata(p >> 12 & 1) - cart_.wramdata(0);
			unsigned addr = (p & 0xFFF) + bank;
			CDMapResult ret = { eCDLog_AddrType_WRAM, addr };
			return ret;
		} else if (p < mm_hram_begin) {
		} else {
			////this is just for debugging, really, it's pretty useless
			//CDMapResult ret = { eCDLog_AddrType_HRAM, (p - mm_hram_begin) };
			//return ret;
		}
		CDMapResult ret = { eCDLog_AddrType_None, p };
		return ret;
	}

	template <bool peek, bool execute, bool opcode, bool callbacksActive>
	unsigned read(unsigned p, unsigned long cc) {
		if (!peek && callbacksActive && !execute && readCallback_)
			readCallback_(p, callbackCycleOffset(cc), 0);
		else if (!peek && callbacksActive && execute && opcode && execCallback_)
			execCallback_(p, callbackCycleOffset(cc), 0);

		if (biosMode_ && p < bios_.size() && !(p >= 0x100 && p < 0x200))
			return readBios(p);
		else if (!peek && callbacksActive && cdCallback_) {
			CDMapResult map = CDMap(p);
			if (map.type != eCDLog_AddrType_None) {
				eCDLog_Flags flags;
				if (execute && opcode)
					flags = eCDLog_Flags_ExecOpcode;
				else if (execute)
					flags = eCDLog_Flags_ExecOperand;
				else
					flags = eCDLog_Flags_Data;

				cdCallback_(map.addr, map.type, flags);
			}
		}

		unsigned char const *const rmem = cart_.rmem(p >> 12);
		unsigned (Memory::*nontrivial_handling)(unsigned, unsigned long) = peek ? &Memory::nontrivial_peek : &Memory::nontrivial_read;
		switch (p >> 13 & 7) {
			case 0:
			case 1:
			case 2:
			case 3:
				if (!peek) {
					cartBus_ = rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
					lastCartBusUpdate_ = cc;
					return cartBus_;
				}
				// fallthrough
			case 4:
				return rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
			case 5:
				if (!cart_.disabledRam() & !cart_.isMbc2() & !cart_.isPocketCamera())
					return rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
				else if (cart_.disabledRam())
					return rmem ? cartBus(cc) : (this->*nontrivial_handling)(p, cc);
				else if (cart_.isMbc2())
					return rmem ? ((rmem[p & 0xA1FF] & 0x0F) | (cartBus(cc) & 0xF0)) : (this->*nontrivial_handling)(p & 0xA1FF, cc);
				else if (!peek && cart_.isPocketCamera() && cart_.cameraIsActive(cc))
					return rmem ? 0x00 : (this->*nontrivial_handling)(p, cc);
				else
					return rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
			case 6:
				if (!peek && !isCgb()) {
					cartBus_ = rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
					lastCartBusUpdate_ = cc;
					return cartBus_;
				}
				return rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
			case 7:
				if (!peek && !isCgb() && p < mm_oam_begin) {
					cartBus_ = rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
					lastCartBusUpdate_ = cc;
					return cartBus_;
				}
				// fallthrough
			default:
				return rmem ? rmem[p] : (this->*nontrivial_handling)(p, cc);
		}
	}

	template <bool poke, bool callbacksActive>
	void write(unsigned p, unsigned data, unsigned long cc) {
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin))
			p &= 0xA1FF;

		if (cart_.wmem(p >> 12))
			cart_.wmem(p >> 12)[p] = data;
		else
			nontrivial_write(p, data, cc);

		if (!poke && callbacksActive) {
			if (writeCallback_)
				writeCallback_(p, callbackCycleOffset(cc), data);

			if (cdCallback_ && !biosMode_) {
				CDMapResult map = CDMap(p);
				if (map.type != eCDLog_AddrType_None)
					cdCallback_(map.addr, map.type, eCDLog_Flags_Data);
			}
		}
	}

	template <bool callbacksActive>
	void ff_write(unsigned p, unsigned data, unsigned long cc) {
		if (p - 0x80u < 0x7Fu)
			ioamhram_[p + 0x100] = data;
		else
			nontrivial_ff_write(p, data, cc);

		if (callbacksActive && writeCallback_)
			writeCallback_(mm_io_begin + p, callbackCycleOffset(cc), data);
		if (callbacksActive && cdCallback_ && !biosMode_) {
			CDMapResult map = CDMap(mm_io_begin + p);
			if (map.type != eCDLog_AddrType_None)
				cdCallback_(map.addr, map.type, eCDLog_Flags_Data);
		}
	}

	unsigned long event(unsigned long cycleCounter);
	unsigned long resetCounters(unsigned long cycleCounter);
	LoadRes loadROM(Array<unsigned char> &buffer, unsigned flags, std::string const &filepath);
	void setSaveDir(std::string const &dir) { cart_.setSaveDir(dir); }

	void setInputGetter(InputGetter *getInput, void *p) {
		getInput_ = getInput;
		getInputP_ = p;
	}

	unsigned getJoypadIndex() {
		return sgb_.getJoypadIndex();
	}

	void setReadCallback(MemoryCallback callback) {
		readCallback_ = callback;
	}

	void setWriteCallback(MemoryCallback callback) {
		writeCallback_ = callback;
	}

	void setExecCallback(MemoryCallback callback) {
		execCallback_ = callback;
	}

	void setCDCallback(CDCallback cdc) {
		cdCallback_ = cdc;
	}

	void setScanlineCallback(void (*callback)(), int sl) {
		lcd_.setScanlineCallback(callback, sl);
	}

	void setLinkCallback(void (*callback)()) {
		linkCallback_ = callback;
	}

	void setCameraCallback(void (*callback)(int *cameraBuf)) {
		if (cart_.isPocketCamera())
			cart_.setCameraCallback(callback);
	}

	void setRemoteCallback(unsigned char (*callback)()) {
		cart_.setRemoteCallback(callback);
	}

	inline bool slowCallbacksActive() { return readCallback_ || writeCallback_ || execCallback_ || cdCallback_; }

	void setEndtime(unsigned long cc, unsigned long inc);

	std::size_t callbackCycleOffset(unsigned long const cc) { return psg_.callbackCycleOffset(cc, isDoubleSpeed()); }

	void setSoundBuffer(uint_least32_t *buf) { psg_.setBuffer(buf); }
	std::size_t fillSoundBuffer(unsigned long cc);

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		lcd_.setVideoBuffer(videoBuf, pitch);
		sgb_.setVideoBuffer(videoBuf, pitch);
	}

	unsigned updateScreenBorder(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		return isSgb()
			? sgb_.updateScreenBorder(videoBuf, pitch)
			: -1;
	}

	unsigned generateSgbSamples(short *soundBuf, std::size_t &samples) {
		return isSgb()
			? sgb_.generateSamples(soundBuf, samples)
			: -1;
	}

	unsigned generateMbcSamples(short *soundBuf) {
		return cart_.generateSamples(soundBuf);
	}

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		if (!isSgb())
			lcd_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}
    
	void blackScreen() {
		lcd_.blackScreen();
	}

	void setCgbPalette(unsigned *lut) {
		if (isSgb())
			sgb_.setCgbPalette(lut);
		else
			lcd_.setCgbPalette(lut);
	}

	void setTimeMode(bool useCycles, unsigned long const cc) { cart_.setTimeMode(useCycles, cc); }
	void setRtcDivisorOffset(long const rtcDivisorOffset) { cart_.setRtcDivisorOffset(rtcDivisorOffset); }

	void setCartBusPullUpTime(unsigned long const cartBusPullUpTime) { cartBusPullUpTime_ = cartBusPullUpTime; }

	int linkStatus(int which);

	void setGameGenie(std::string const &codes) { cart_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { interrupter_.setGameShark(codes); }
	void updateInput();

	void setBios(Array<unsigned char> &buffer) {
		bios_.reset(buffer.size());
		std::memcpy(bios_.get(), buffer.get(), buffer.size());
	}

	unsigned long long timeNow() const { return cart_.timeNow(); }
	void setTime(unsigned long long dividers) { cart_.setTime(dividers); }

	unsigned long getDivLastUpdate() { return divLastUpdate_; }
	unsigned char getRawIOAMHRAM(int offset) { return ioamhram_[offset]; }

	void setSpeedupFlags(unsigned flags) {
		lcd_.setSpeedupFlags(flags);
		psg_.setSpeedupFlags(flags);
	}

	template<bool isReader>void SyncState(NewState *ns);

private:
	Cartridge cart_;
	Sgb sgb_;
	unsigned char ioamhram_[0x200];
	Array<unsigned char> bios_;
	InputGetter *getInput_;
	void *getInputP_;
	unsigned long divLastUpdate_;
	unsigned long lastOamDmaUpdate_;
	unsigned long lastCartBusUpdate_;
	unsigned long cartBusPullUpTime_; 
	InterruptRequester intreq_;
	Tima tima_;
	LCD lcd_;
	PSG psg_;
	Interrupter interrupter_;
	unsigned short dmaSource_;
	unsigned short dmaDestination_;
	unsigned char oamDmaPos_;
	unsigned char oamDmaStartPos_;
	unsigned char serialCnt_;
	unsigned char cartBus_;
	bool blanklcd_;
	bool biosMode_;
	bool agbFlag_;
	bool gbIsSgb_;
	bool stopped_;
	enum HdmaState { hdma_low, hdma_high, hdma_requested } haltHdmaState_;

	MemoryCallback readCallback_;
	MemoryCallback writeCallback_;
	MemoryCallback execCallback_;
	CDCallback cdCallback_;
	void (*linkCallback_)();
	bool linked_;
	bool linkClockTrigger_;

	void decEventCycles(IntEventId eventId, unsigned long dec);
	void oamDmaInitSetup();
	void updateOamDma(unsigned long cycleCounter);
	void startOamDma(unsigned long cycleCounter);
	void endOamDma(unsigned long cycleCounter);
	unsigned char const * oamDmaSrcPtr() const;
	unsigned long dma(unsigned long cc);
	unsigned nontrivial_ff_read(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_read(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_ff_peek(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_peek(unsigned p, unsigned long cycleCounter);
	void nontrivial_ff_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void nontrivial_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void updateSerial(unsigned long cc);
	void updateTimaIrq(unsigned long cc);
	void updateIrqs(unsigned long cc);
	bool isDoubleSpeed() const { return lcd_.isDoubleSpeed(); }
	unsigned char cartBus(unsigned long const cc) const {
		unsigned long const diff = cc - lastCartBusUpdate_;
		return diff >= (cartBusPullUpTime_ << isDoubleSpeed()) ? 0xFF : cartBus_;
	}
};

}

#endif
