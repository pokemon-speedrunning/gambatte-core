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

#include "cartridge.h"
#include "file/file.h"
#include "../savestate.h"
#include "pakinfo_internal.h"

#include <cstring>
#include <fstream>
#include <zlib.h>
//#include <stdio.h>

using namespace gambatte;

namespace {

std::string stripExtension(std::string const &str) {
	std::string::size_type const lastDot = str.find_last_of('.');
	std::string::size_type const lastSlash = str.find_last_of('/');

	if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastSlash < lastDot))
		return str.substr(0, lastDot);

	return str;
}

std::string stripDir(std::string const &str) {
	std::string::size_type const lastSlash = str.find_last_of('/');
	if (lastSlash != std::string::npos)
		return str.substr(lastSlash + 1);

	return str;
}

void enforce8bit(unsigned char *data, std::size_t size) {
	if (static_cast<unsigned char>(0x100))
		while (size--)
			*data++ &= 0xFF;
}

unsigned pow2ceil(unsigned n) {
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	++n;

	return n;
}

bool presumedMulti64Mbc1(unsigned char const header[], unsigned rombanks) {
	return header[0x147] == 1 && header[0x149] == 0 && rombanks == 64;
}

bool hasBattery(unsigned char headerByte0x147) {
	switch (headerByte0x147) {
	case 0x03:
	case 0x06:
	case 0x09:
	case 0x0F:
	case 0x10:
	case 0x13:
	case 0x1B:
	case 0x1E:
	case 0xFC:
	case 0xFE:
	case 0xFF:
		return true;
	}

	return false;
}

bool hasRtc(unsigned headerByte0x147) {
	switch (headerByte0x147) {
	case 0x0F:
	case 0x10:
	case 0xFE:
		return true;
	}

	return false;
}

int asHex(char c) {
	return c >= 'A' ? c - 'A' + 0xA : c - '0';
}

}

Cartridge::Cartridge()
: mbc2_(false)
, huc1_(false)
, pocketCamera_(false)
, rtc_(time_)
, huc3_(time_)
{
}

void Cartridge::setStatePtrs(SaveState &state) {
	state.mem.vram.set(memptrs_.vramdata(), memptrs_.vramdataend() - memptrs_.vramdata());
	state.mem.sram.set(memptrs_.rambankdata(), memptrs_.rambankdataend() - memptrs_.rambankdata());
	state.mem.wram.set(memptrs_.wramdata(0), memptrs_.wramdataend() - memptrs_.wramdata(0));

	camera_.setStatePtrs(state);
}

void Cartridge::saveState(SaveState &state, unsigned long const cc) {
	state.mem.HuC3RAMflag = 0; // let's not leave random data here
	mbc_->saveState(state.mem);
	if (!isHuC3())
		rtc_.update(cc);

	time_.saveState(state, cc, isHuC3());
	rtc_.saveState(state);
	huc3_.saveState(state);
	camera_.saveState(state);
}

void Cartridge::loadState(SaveState const &state) {
	camera_.loadState(state, isCgb());
	huc3_.loadState(state);
	rtc_.loadState(state);
	time_.loadState(state, isHuC3(), isCgb());
	mbc_->loadState(state.mem);
}

std::string const Cartridge::saveBasePath() const {
	return saveDir_.empty()
	     ? defaultSaveBasePath_
	     : saveDir_ + stripDir(defaultSaveBasePath_);
}

void Cartridge::setSaveDir(std::string const &dir) {
	saveDir_ = dir;
	if (!saveDir_.empty() && saveDir_[saveDir_.length() - 1] != '/')
		saveDir_ += '/';
}

LoadRes Cartridge::loadROM(std::string const &romfile,
                           bool const cgbMode,
                           bool const multicartCompat)
{
	if (romfile.empty()) {
		mbc_.reset();
		return LOADRES_IO_ERROR;
	}

	scoped_ptr<File> const rom(newFileInstance(romfile));
	if (rom->fail())
		return LOADRES_IO_ERROR;

	enum Cartridgetype { type_plain,
	                     type_mbc1,
	                     type_mbc2,
	                     type_mbc3,
	                     type_mbc5,
	                     type_huc1,
	                     type_huc3,
	                     type_pocketcamera,
	                     type_wisdomtree };
	Cartridgetype type = type_plain;
	unsigned rambanks = 1;
	unsigned rombanks = 2;
	bool cgb = false;

	{
		unsigned char header[0x150];
		rom->read(reinterpret_cast<char *>(header), sizeof header);

		switch (header[0x0147]) {
		case 0x00: type = type_plain; break;
		case 0x01:
		case 0x02:
		case 0x03: type = type_mbc1; break;
		case 0x05:
		case 0x06: type = type_mbc2; break;
		case 0x08:
		case 0x09: type = type_plain; break;
		case 0x0B:
		case 0x0C:
		case 0x0D: return LOADRES_UNSUPPORTED_MBC_MMM01;
		case 0x0F:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13: type = type_mbc3; break;
		case 0x1B:
			if (multicartCompat && header[0x014A] == 0xE1)
				return LOADRES_UNSUPPORTED_MBC_EMS_MULTICART;
			else {
				type = type_mbc5;
				break;
			}
		case 0x19:
		case 0x1A:
		case 0x1C:
		case 0x1D:
		case 0x1E: type = type_mbc5; break;
		case 0x20: return LOADRES_UNSUPPORTED_MBC_MBC6;
		case 0x22: return LOADRES_UNSUPPORTED_MBC_MBC7;
		case 0xBE:
			if (multicartCompat)
				return LOADRES_UNSUPPORTED_MBC_BUNG_MULTICART;
			else
				return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		case 0xFC: type = type_pocketcamera; break;
		case 0xFD: return LOADRES_UNSUPPORTED_MBC_TAMA5;
		case 0xFE: type = type_huc3; break;
		case 0xFF: type = type_huc1; break;
		case 0xC0:
			if (multicartCompat && header[0x014A] == 0xD1) {
				type = type_wisdomtree;
				break;
			} else
				return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		default:   return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		}

		/*switch (header[0x0148]) {
		case 0x00: rombanks = 2; break;
		case 0x01: rombanks = 4; break;
		case 0x02: rombanks = 8; break;
		case 0x03: rombanks = 16; break;
		case 0x04: rombanks = 32; break;
		case 0x05: rombanks = 64; break;
		case 0x06: rombanks = 128; break;
		case 0x07: rombanks = 256; break;
		case 0x08: rombanks = 512; break;
		case 0x52: rombanks = 72; break;
		case 0x53: rombanks = 80; break;
		case 0x54: rombanks = 96; break;
		default: return -1;
		}*/

		rambanks = numRambanksFromH14x(header[0x147], header[0x149]);
		cgb = cgbMode;
	}

	std::size_t const filesize = rom->size();
	rombanks = std::max(pow2ceil(filesize / rombank_size()), 2u);

	if (multicartCompat && type == type_plain && rombanks > 2)
		type = type_wisdomtree; // todo: better hack than this (probably should just use crc32s?)

	defaultSaveBasePath_.clear();
	ggUndoList_.clear();
	mbc_.reset();
	memptrs_.reset(rombanks, rambanks, cgb ? 8 : 2);
	rtc_.set(false, 0);
	huc3_.set(false);
	camera_.set(NULL);
	mbc2_ = false;
	huc1_ = false;
	pocketCamera_ = false;

	rom->rewind();
	rom->read(reinterpret_cast<char*>(memptrs_.romdata()), filesize / rombank_size() * rombank_size());
	std::memset(memptrs_.romdata() + filesize / rombank_size() * rombank_size(),
	            0xFF,
	            (rombanks - filesize / rombank_size()) * rombank_size());
	enforce8bit(memptrs_.romdata(), rombanks * rombank_size());

	if (rom->fail())
		return LOADRES_IO_ERROR;

	defaultSaveBasePath_ = stripExtension(romfile);

	switch (type) {
	case type_plain: mbc_.reset(new Mbc0(memptrs_)); break;
	case type_mbc1:
		if (multicartCompat && presumedMulti64Mbc1(memptrs_.romdata(), rombanks)) {
			mbc_.reset(new Mbc1Multi64(memptrs_));
		} else
			mbc_.reset(new Mbc1(memptrs_));

		break;
	case type_mbc2: mbc_.reset(new Mbc2(memptrs_)); mbc2_ = true; break;
	case type_mbc3:
		{
			bool mbc30 = rombanks > 0x80 || rambanks > 0x04;
			Rtc *rtc = hasRtc(memptrs_.romdata()[0x147]) ? &rtc_ : 0;
			if(mbc30)
				mbc_.reset(new Mbc30(memptrs_, rtc));
			else
				mbc_.reset(new Mbc3 (memptrs_, rtc));
		}
		break;
	case type_mbc5: mbc_.reset(new Mbc5(memptrs_)); break;
	case type_huc1: mbc_.reset(new HuC1(memptrs_)); huc1_ = true; break;
	case type_huc3:
		huc3_.set(true);
		mbc_.reset(new HuC3(memptrs_, &huc3_));
		break;
	case type_pocketcamera: mbc_.reset(new PocketCamera(memptrs_, &camera_)); pocketCamera_ = true; break;
	case type_wisdomtree: mbc_.reset(new WisdomTree(memptrs_)); break;
	}

	return LOADRES_OK;
}

LoadRes Cartridge::loadROM(char const *romfiledata,
                           unsigned romfilelength,
						   bool const cgbMode,
						   bool const multicartCompat)
{
	enum Cartridgetype { type_plain,
	                     type_mbc1,
	                     type_mbc2,
	                     type_mbc3,
	                     type_mbc5,
	                     type_huc1,
	                     type_huc3,
	                     type_pocketcamera,
	                     type_wisdomtree };
	Cartridgetype type = type_plain;
	unsigned rambanks = 1;
	unsigned rombanks = 2;
	bool cgb = false;

	{
		unsigned char header[0x150];
		if (romfilelength >= sizeof header)
			std::memcpy(header, romfiledata, sizeof header);
		else
			return LOADRES_IO_ERROR;

		switch (header[0x0147]) {
		case 0x00: type = type_plain; break;
		case 0x01:
		case 0x02:
		case 0x03: type = type_mbc1; break;
		case 0x05:
		case 0x06: type = type_mbc2; break;
		case 0x08:
		case 0x09: type = type_plain; break;
		case 0x0B:
		case 0x0C:
		case 0x0D: return LOADRES_UNSUPPORTED_MBC_MMM01;
		case 0x0F:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13: type = type_mbc3; break;
		case 0x1B:
			if (multicartCompat && header[0x014A] == 0xE1)
				return LOADRES_UNSUPPORTED_MBC_EMS_MULTICART;
			else {
				type = type_mbc5;
				break;
			}
		case 0x19:
		case 0x1A:
		case 0x1C:
		case 0x1D:
		case 0x1E: type = type_mbc5; break;
		case 0x20: return LOADRES_UNSUPPORTED_MBC_MBC6;
		case 0x22: return LOADRES_UNSUPPORTED_MBC_MBC7;
		case 0xBE:
			if (multicartCompat)
				return LOADRES_UNSUPPORTED_MBC_BUNG_MULTICART;
			else
				return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		case 0xFC: type = type_pocketcamera; break;
		case 0xFD: return LOADRES_UNSUPPORTED_MBC_TAMA5;
		case 0xFE: type = type_huc3; break;
		case 0xFF: type = type_huc1; break;
		case 0xC0:
			if (header[0x014A] == 0xD1) {
				type = type_wisdomtree;
				break;
			} else
				return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		default:   return LOADRES_BAD_FILE_OR_UNKNOWN_MBC;
		}

		/*switch (header[0x0148]) {
		case 0x00: rombanks = 2; break;
		case 0x01: rombanks = 4; break;
		case 0x02: rombanks = 8; break;
		case 0x03: rombanks = 16; break;
		case 0x04: rombanks = 32; break;
		case 0x05: rombanks = 64; break;
		case 0x06: rombanks = 128; break;
		case 0x07: rombanks = 256; break;
		case 0x08: rombanks = 512; break;
		case 0x52: rombanks = 72; break;
		case 0x53: rombanks = 80; break;
		case 0x54: rombanks = 96; break;
		default: return -1;
		}*/

		rambanks = numRambanksFromH14x(header[0x147], header[0x149]);
		cgb = cgbMode;
	}
	std::size_t const filesize = romfilelength;
	rombanks = std::max(pow2ceil(filesize / rombank_size()), 2u);

	if (multicartCompat && type == type_plain && rombanks > 2)
		type = type_wisdomtree; // todo: better hack than this (probably should just use crc32s?)

	mbc_.reset();
	memptrs_.reset(rombanks, rambanks, cgb ? 8 : 2);
	rtc_.set(false, 0);
	huc3_.set(false);
	camera_.set(NULL);
	mbc2_ = false;
	huc1_ = false;
	pocketCamera_ = false;

	std::memcpy(memptrs_.romdata(), romfiledata, (filesize / rombank_size() * rombank_size()));
	std::memset(memptrs_.romdata() + filesize / rombank_size() * rombank_size(),
	            0xFF,
	            (rombanks - filesize / rombank_size()) * rombank_size());
	enforce8bit(memptrs_.romdata(), rombanks * rombank_size());

	switch (type) {
	case type_plain: mbc_.reset(new Mbc0(memptrs_)); break;
	case type_mbc1:
		if (multicartCompat && presumedMulti64Mbc1(memptrs_.romdata(), rombanks)) {
			mbc_.reset(new Mbc1Multi64(memptrs_));
		} else
			mbc_.reset(new Mbc1(memptrs_));

		break;
	case type_mbc2: mbc_.reset(new Mbc2(memptrs_)); mbc2_ = true; break;
	case type_mbc3:
		{
			bool mbc30 = rombanks > 0x80 || rambanks > 0x04;
			Rtc *rtc = hasRtc(memptrs_.romdata()[0x147]) ? &rtc_ : 0;
			if(mbc30)
				mbc_.reset(new Mbc30(memptrs_, rtc));
			else
				mbc_.reset(new Mbc3 (memptrs_, rtc));
		}
		break;
	case type_mbc5: mbc_.reset(new Mbc5(memptrs_)); break;
	case type_huc1: mbc_.reset(new HuC1(memptrs_)); huc1_ = true; break;
	case type_huc3:
		huc3_.set(true);
		mbc_.reset(new HuC3(memptrs_, &huc3_));
		break;
	case type_pocketcamera: mbc_.reset(new PocketCamera(memptrs_, &camera_)); pocketCamera_ = true; break;
	case type_wisdomtree: mbc_.reset(new WisdomTree(memptrs_)); break;
	}

	return LOADRES_OK;
}

enum { Dh = 0, Dl = 1, H = 2, M = 3, S = 4, C = 5, L = 6 };

int Cartridge::saveSavedataLength(bool isDeterministic) {
	int ret = 0;
	if (hasBattery(memptrs_.romdata()[0x147])) {
		ret = memptrs_.rambankdataend() - memptrs_.rambankdata();
	}
	if (hasRtc(memptrs_.romdata()[0x147]) && !isDeterministic) {
		ret += isHuC3() ? 8 : (8 + 14);
	}
	return ret;
}

void Cartridge::loadSavedata(unsigned long const cc) {
	std::string const &sbp = saveBasePath();

	if (hasBattery(memptrs_.romdata()[0x147])) {
		std::ifstream file((sbp + ".sav").c_str(), std::ios::binary | std::ios::in);

		if (file.is_open()) {
			file.read(reinterpret_cast<char*>(memptrs_.rambankdata()),
			          memptrs_.rambankdataend() - memptrs_.rambankdata());
			enforce8bit(memptrs_.rambankdata(), memptrs_.rambankdataend() - memptrs_.rambankdata());
		}
	}

	if (hasRtc(memptrs_.romdata()[0x147])) {
		std::ifstream file((sbp + ".rtc").c_str(), std::ios::binary | std::ios::in);
		if (file) {
			timeval baseTime;
			baseTime.tv_sec = file.get() & 0xFF;
			baseTime.tv_sec = baseTime.tv_sec << 8 | (file.get() & 0xFF);
			baseTime.tv_sec = baseTime.tv_sec << 8 | (file.get() & 0xFF);
			baseTime.tv_sec = baseTime.tv_sec << 8 | (file.get() & 0xFF);
			baseTime.tv_usec = file.get() & 0xFF;

			if (!file.eof()) {
				baseTime.tv_usec = baseTime.tv_usec << 8 | (file.get() & 0xFF);
				baseTime.tv_usec = baseTime.tv_usec << 8 | (file.get() & 0xFF);
				baseTime.tv_usec = baseTime.tv_usec << 8 | (file.get() & 0xFF);
			} else
				baseTime.tv_usec = 0;
				
			if (baseTime.tv_sec > Time::now().tv_sec) // prevent malformed RTC files from giving negative times
				baseTime = Time::now();
				
			if (isHuC3())
				time_.setBaseTime(baseTime, cc);
			else {
				unsigned long rtcRegs [11] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
				rtcRegs[Dh] = file.get() & 0xC1;
				if (!file.eof()) {
					rtcRegs[Dl] = file.get() & 0xFF;
					rtcRegs[H] = file.get() & 0x1F;
					rtcRegs[M] = file.get() & 0x3F;
					rtcRegs[S] = file.get() & 0x3F;
					rtcRegs[C] = file.get() & 0xFF;
					rtcRegs[C] = rtcRegs[C] << 8 | (file.get() & 0xFF);
					rtcRegs[C] = rtcRegs[C] << 8 | (file.get() & 0xFF);
					rtcRegs[C] = rtcRegs[C] << 8 | (file.get() & 0xFF);
					rtcRegs[Dh+L] = file.get() & 0xC1;
					rtcRegs[Dl+L] = file.get() & 0xFF;
					rtcRegs[H+L] = file.get() & 0x1F;
					rtcRegs[M+L] = file.get() & 0x3F;
					rtcRegs[S+L] = file.get() & 0x3F;
				}
				else
					rtcRegs[Dh] = 0;

				setRtcRegs(rtcRegs);
				rtc_.setBaseTime(baseTime, cc);
			}
		}
	}
}

void Cartridge::saveSavedata(unsigned long const cc) {
	std::string const &sbp = saveBasePath();

	if (hasBattery(memptrs_.romdata()[0x147])) {
		std::ofstream file((sbp + ".sav").c_str(), std::ios::binary | std::ios::out);
		file.write(reinterpret_cast<char const *>(memptrs_.rambankdata()),
		           memptrs_.rambankdataend() - memptrs_.rambankdata());
	}

	if (hasRtc(memptrs_.romdata()[0x147])) {
		std::ofstream file((sbp + ".rtc").c_str(), std::ios::binary | std::ios::out);
		timeval baseTime = time_.baseTime(cc, isHuC3());
		file.put(baseTime.tv_sec  >> 24 & 0xFF);
		file.put(baseTime.tv_sec  >> 16 & 0xFF);
		file.put(baseTime.tv_sec  >>  8 & 0xFF);
		file.put(baseTime.tv_sec        & 0xFF);
		file.put(baseTime.tv_usec >> 24 & 0xFF);
		file.put(baseTime.tv_usec >> 16 & 0xFF);
		file.put(baseTime.tv_usec >>  8 & 0xFF);
		file.put(baseTime.tv_usec       & 0xFF);
		if (!isHuC3()) {
			unsigned long rtcRegs [11];
			getRtcRegs(rtcRegs, cc);
			file.put(rtcRegs[Dh]      & 0xC1);
			file.put(rtcRegs[Dl]      & 0xFF);
			file.put(rtcRegs[H]       & 0x1F);
			file.put(rtcRegs[M]       & 0x3F);
			file.put(rtcRegs[S]       & 0x3F);
			file.put(rtcRegs[C] >> 24 & 0xFF);
			file.put(rtcRegs[C] >> 16 & 0xFF);
			file.put(rtcRegs[C] >>  8 & 0xFF);
			file.put(rtcRegs[C]       & 0xFF);
			file.put(rtcRegs[Dh+L]    & 0xC1);
			file.put(rtcRegs[Dl+L]    & 0xFF);
			file.put(rtcRegs[H+L]     & 0x1F);
			file.put(rtcRegs[M+L]     & 0x3F);
			file.put(rtcRegs[S+L]     & 0x3F);
		}
	}
}

void Cartridge::saveSavedata(char* dest, unsigned long const cc, bool isDeterministic) {
	if (hasBattery(memptrs_.romdata()[0x147])) {
		int length = memptrs_.rambankdataend() - memptrs_.rambankdata();
		std::memcpy(dest, memptrs_.rambankdata(), length);
		dest += length;
	}

	if (hasRtc(memptrs_.romdata()[0x147]) && !isDeterministic) {
		timeval basetime = time_.baseTime(cc, isHuC3());
		*dest++ = (basetime.tv_sec  >> 24 & 0xFF);
		*dest++ = (basetime.tv_sec  >> 16 & 0xFF);
		*dest++ = (basetime.tv_sec  >>  8 & 0xFF);
		*dest++ = (basetime.tv_sec        & 0xFF);
		*dest++ = (basetime.tv_usec >> 24 & 0xFF);
		*dest++ = (basetime.tv_usec >> 16 & 0xFF);
		*dest++ = (basetime.tv_usec >>  8 & 0xFF);
		*dest++ = (basetime.tv_usec       & 0xFF);
		if (!isHuC3()) {
			unsigned long rtcRegs[11];
			getRtcRegs(rtcRegs, cc);
			*dest++ = (rtcRegs[Dh]      & 0xC1);
			*dest++ = (rtcRegs[Dl]      & 0xFF);
			*dest++ = (rtcRegs[H]       & 0x1F);
			*dest++ = (rtcRegs[M]       & 0x3F);
			*dest++ = (rtcRegs[S]       & 0x3F);
			*dest++ = (rtcRegs[C] >> 24 & 0xFF);
			*dest++ = (rtcRegs[C] >> 16 & 0xFF);
			*dest++ = (rtcRegs[C] >>  8 & 0xFF);
			*dest++ = (rtcRegs[C]       & 0xFF);
			*dest++ = (rtcRegs[Dh+L]    & 0xC1);
			*dest++ = (rtcRegs[Dl+L]    & 0xFF);
			*dest++ = (rtcRegs[H+L]     & 0x1F);
			*dest++ = (rtcRegs[M+L]     & 0x3F);
			*dest++ = (rtcRegs[S+L]     & 0x3F);
		}
	}
}

void Cartridge::loadSavedata(char const *data, unsigned long const cc, bool isDeterministic) {
	if (hasBattery(memptrs_.romdata()[0x147])) {
		int length = memptrs_.rambankdataend() - memptrs_.rambankdata();
		std::memcpy(memptrs_.rambankdata(), data, length);
		data += length;
		enforce8bit(memptrs_.rambankdata(), length);
	}

	if (hasRtc(memptrs_.romdata()[0x147]) && !isDeterministic) {
		timeval basetime;
		basetime.tv_sec = (*data++ & 0xFF);
		basetime.tv_sec = basetime.tv_sec << 8 | (*data++ & 0xFF);
		basetime.tv_sec = basetime.tv_sec << 8 | (*data++ & 0xFF);
		basetime.tv_sec = basetime.tv_sec << 8 | (*data++ & 0xFF);
		basetime.tv_usec = (*data++ & 0xFF);
		basetime.tv_usec = basetime.tv_usec << 8 | (*data++ & 0xFF);
		basetime.tv_usec = basetime.tv_usec << 8 | (*data++ & 0xFF);
		basetime.tv_usec = basetime.tv_usec << 8 | (*data++ & 0xFF);

		if (basetime.tv_sec > Time::now().tv_sec) // prevent malformed save files from giving negative times
			basetime = Time::now();

		if (isHuC3())
			time_.setBaseTime(basetime, cc);
		else {
			unsigned long rtcRegs [11];
			rtcRegs[Dh] = *data++ & 0xC1;
			rtcRegs[Dl] = *data++ & 0xFF;
			rtcRegs[H] = *data++ & 0x1F;
			rtcRegs[M] = *data++ & 0x3F;
			rtcRegs[S] = *data++ & 0x3F;
			rtcRegs[C] = *data++ & 0xFF;
			rtcRegs[C] = rtcRegs[C] << 8 | (*data++ & 0xFF);
			rtcRegs[C] = rtcRegs[C] << 8 | (*data++ & 0xFF);
			rtcRegs[C] = rtcRegs[C] << 8 | (*data++ & 0xFF);
			rtcRegs[Dh+L] = *data++ & 0xC1;
			rtcRegs[Dl+L] = *data++ & 0xFF;
			rtcRegs[H+L] = *data++ & 0x1F;
			rtcRegs[M+L] = *data++ & 0x3F;
			rtcRegs[S+L] = *data++ & 0x3F;
			setRtcRegs(rtcRegs);
			rtc_.setBaseTime(basetime, cc);
		}
	}
}

bool Cartridge::getMemoryArea(int which, unsigned char **data, int *length) const {
	if (!data || !length)
		return false;

	switch (which) {
	case 0:
		*data = memptrs_.vramdata();
		*length = memptrs_.vramdataend() - memptrs_.vramdata();
		return true;
	case 1:
		*data = memptrs_.romdata();
		*length = memptrs_.romdataend() - memptrs_.romdata();
		return true;
	case 2:
		*data = memptrs_.wramdata(0);
		*length = memptrs_.wramdataend() - memptrs_.wramdata(0);
		return true;
	case 3:
		*data = memptrs_.rambankdata();
		*length = memptrs_.rambankdataend() - memptrs_.rambankdata();
		return true;
	default:
		return false;
	}
}

void Cartridge::applyGameGenie(std::string const &code) {
	if (6 < code.length()) {
		unsigned const val = (asHex(code[0]) << 4 | asHex(code[1])) & 0xFF;
		unsigned const addr = (    asHex(code[2])        <<  8
		                        |  asHex(code[4])        <<  4
		                        |  asHex(code[5])
		                        | (asHex(code[6]) ^ 0xF) << 12) & 0x7FFF;
		unsigned cmp = 0xFFFF;
		if (10 < code.length()) {
			cmp = (asHex(code[8]) << 4 | asHex(code[10])) ^ 0xFF;
			cmp = ((cmp >> 2 | cmp << 6) ^ 0x45) & 0xFF;
		}

		for (unsigned bank = 0; bank < rombanks(memptrs_); ++bank) {
			if (mbc_->isAddressWithinAreaRombankCanBeMappedTo(addr, bank)
					&& (cmp > 0xFF || memptrs_.romdata()[bank * rombank_size() + addr % rombank_size()] == cmp)) {
				ggUndoList_.push_back(AddrData(bank * rombank_size() + addr % rombank_size(),
				                      memptrs_.romdata()[bank * rombank_size() + addr % rombank_size()]));
				memptrs_.romdata()[bank * rombank_size() + addr % rombank_size()] = val;
			}
		}
	}
}

void Cartridge::setGameGenie(std::string const &codes) {
	if (loaded()) {
		for (std::vector<AddrData>::reverse_iterator it =
				ggUndoList_.rbegin(), end = ggUndoList_.rend(); it != end; ++it) {
			if (memptrs_.romdata() + it->addr < memptrs_.romdataend())
				memptrs_.romdata()[it->addr] = it->data;
		}

		ggUndoList_.clear();

		std::string code;
		for (std::size_t pos = 0; pos < codes.length(); pos += code.length() + 1) {
			code = codes.substr(pos, codes.find(';', pos) - pos);
			applyGameGenie(code);
		}
	}
}

PakInfo const Cartridge::pakInfo(bool const multipakCompat) const {
	if (loaded()) {
		unsigned crc = 0L;
		unsigned const rombs = rombanks(memptrs_);
		crc = crc32(crc, memptrs_.romdata(), rombs*0x4000ul);
		return PakInfo(multipakCompat && presumedMulti64Mbc1(memptrs_.romdata(), rombs),
		               rombs,
			       crc,
		               memptrs_.romdata());
	}

	return PakInfo();
}

SYNCFUNC(Cartridge) {
	SSS(memptrs_);
	SSS(time_);
	SSS(rtc_);
	SSS(huc3_);
	SSS(camera_);
	TSS(mbc_);
}
