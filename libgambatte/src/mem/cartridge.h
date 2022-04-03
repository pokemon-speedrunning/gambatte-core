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

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "mbc/mbc.h"
#include "loadres.h"
#include "savestate.h"
#include "scoped_ptr.h"
#include "array.h"
#include "newstate.h"

#include <string>
#include <vector>

namespace gambatte {

class Cartridge {
public:
	Cartridge();
	void setStatePtrs(SaveState &);
	void saveState(SaveState &, unsigned long cycleCounter);
	void loadState(SaveState const &);
	bool loaded() const { return mbc_.get(); }
	unsigned char const * rmem(unsigned area) const { return memptrs_.rmem(area); }
	unsigned char * wmem(unsigned area) const { return memptrs_.wmem(area); }
	unsigned char * vramdata() const { return memptrs_.vramdata(); }
	unsigned char * romdata(unsigned area) const { return memptrs_.romdata(area); }
	unsigned char * wramdata(unsigned area) const { return memptrs_.wramdata(area); }
	unsigned char const * rdisabledRam() const { return memptrs_.rdisabledRam(); }
	unsigned char const * rsrambankptr() const { return memptrs_.rsrambankptr(); }
	unsigned char * wsrambankptr() const { return memptrs_.wsrambankptr(); }
	unsigned char * vrambankptr() const { return memptrs_.vrambankptr(); }
	OamDmaSrc oamDmaSrc() const { return memptrs_.oamDmaSrc(); }
	bool isInOamDmaConflictArea(unsigned p) const { return memptrs_.isInOamDmaConflictArea(p); }
	unsigned getBank(unsigned type) { return memptrs_.getBank(type); }
	unsigned getAddrBank(unsigned short addr) { return memptrs_.getAddrBank(addr); }
	void setBank(unsigned type, unsigned bank) { memptrs_.setBank(type, bank); }
	void setAddrBank(unsigned short addr, unsigned bank) { memptrs_.setAddrBank(addr, bank); }
	void setVrambank(unsigned bank) { memptrs_.setVrambank(bank); }
	void setWrambank(unsigned bank) { memptrs_.setWrambank(bank); }
	void setOamDmaSrc(OamDmaSrc oamDmaSrc) { memptrs_.setOamDmaSrc(oamDmaSrc); }
	bool disabledRam() const { return mbc_->disabledRam(); }
	void mbcWrite(unsigned addr, unsigned data, unsigned long const cc) { mbc_->romWrite(addr, data, cc); }
	bool isCgb() const { return gambatte::isCgb(memptrs_); }
	void resetCc(unsigned long const oldCc, unsigned long const newCc) {
		time_.resetCc(oldCc, newCc);
		huc3_.resetCc(oldCc, newCc);
		camera_.resetCc(oldCc, newCc);
	}
	void speedChange(unsigned long const cc) {
		time_.speedChange(cc);
		huc3_.speedChange(cc);
		camera_.speedChange(cc);
	}
	void setTimeMode(bool useCycles, unsigned long const cc) { time_.setTimeMode(useCycles, cc); }
	void setRtcDivisorOffset(long const rtcDivisorOffset) { time_.setRtcDivisorOffset(rtcDivisorOffset); }
	unsigned timeNow() const { return time_.timeNow(); }
	void getRtcRegs(unsigned long *dest, unsigned long cc) { rtc_.getRtcRegs(dest, cc); }
	void setRtcRegs(unsigned long *src) { rtc_.setRtcRegs(src); }
	void rtcWrite(unsigned data, unsigned long const cc) { rtc_.write(data, cc); }
	unsigned char rtcRead() const { return rtc_.activeLatch() ? *rtc_.activeLatch() : 0xFF; }
	void loadSavedata(unsigned long cycleCounter);
	void saveSavedata(unsigned long cycleCounter);
	int saveSavedataLength(bool isDeterministic);
	void loadSavedata(char const *data, unsigned long cycleCounter, bool isDeterministic);
	void saveSavedata(char *dest, unsigned long cycleCounter, bool isDeterministic);
	bool getMemoryArea(int which, unsigned char **data, int *length) const;
	std::string const saveBasePath() const;
	void setSaveDir(std::string const &dir);
	LoadRes loadROM(Array<unsigned char> &buffer, bool cgbMode, std::string const &filepath);
	char const * romTitle() const { return reinterpret_cast<char const *>(romHeader + 0x134); }
	class PakInfo const pakInfo() const;
	void setGameGenie(std::string const &codes);
	bool isMbc2() const { return mbc2_; }
	bool isHuC1() const { return huc1_; }
	bool getIrTrigger() const { return ir_.getIrTrigger(); }
	void ackIrTrigger() { ir_.ackIrTrigger(); }
	bool getIrSignal(Infrared::WhichIrGb which) const { return ir_.getIrSignal(which); }
	void setIrSignal(Infrared::WhichIrGb which, bool signal) { ir_.setIrSignal(which, signal); }
	bool isHuC3() const { return huc3_.isHuC3(); }
	void accumulateSamples(unsigned long const cc) { huc3_.accumulateSamples(cc); }
	unsigned generateSamples(short *soundbuf) { return huc3_.generateSamples(soundbuf); }
	void getHuC3Regs(unsigned char *dest, unsigned long cc) { huc3_.getHuC3Regs(dest, cc); }
	void setHuC3Regs(unsigned char *src) { huc3_.setHuC3Regs(src); }
	unsigned char HuC3Read(unsigned p, unsigned long const cc) { return huc3_.read(p, cc); }
	void HuC3Write(unsigned p, unsigned data, unsigned long const cc) { huc3_.write(p, data, cc); }
	bool isPocketCamera() const { return pocketCamera_; }
	bool cameraIsActive(unsigned long cc) { return camera_.cameraIsActive(cc); }
	unsigned char cameraRead(unsigned p, unsigned long const cc) { return camera_.read(p, cc); }
	void cameraWrite(unsigned p, unsigned data, unsigned long const cc) { camera_.write(p, data, cc); }
	void setCameraCallback(bool(*callback)(int32_t *cameraBuf)) { camera_.setCameraCallback(callback); }
	template<bool isReader>void SyncState(NewState *ns);

private:
	unsigned char romHeader[0x150];
	bool mbc2_;
	bool huc1_;
	bool pocketCamera_;
	struct AddrData {
		unsigned long addr;
		unsigned char data;
		AddrData(unsigned long addr, unsigned data) : addr(addr), data(data) {}
	};

	MemPtrs memptrs_;
	Time time_;
	Rtc rtc_;
	Infrared ir_;
	HuC3Chip huc3_;
	Camera camera_;
	scoped_ptr<Mbc> mbc_;
	std::string defaultSaveBasePath_;
	std::string saveDir_;
	std::vector<AddrData> ggUndoList_;

	void applyGameGenie(std::string const &code);
};

}

#endif
