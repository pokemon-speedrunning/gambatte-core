#include "pakinfo_internal.h"

#include <cstring>

namespace gambatte {

enum { flag_mbc1m = 1, flag_header_checksum_ok = 2, flag_m161 = 4, flag_mmm01 = 8, flag_wisdom_tree = 16 };

bool isHeaderChecksumOk(unsigned const char header[]) {
	unsigned csum = 0;
	for (int i = 0x134; i < 0x14D; ++i)
		csum -= header[i] + 1;

	return (csum & 0xFF) == header[0x14D];
}

static bool isMbc2(unsigned char h147) { return h147 == 5 || h147 == 6; }

unsigned numRambanksFromH14x(unsigned char h147, unsigned char h149) {
	switch (h149) {
	case 0x00: return isMbc2(h147) ? 1 : 0;
	case 0x01:
	case 0x02: return 1;
	case 0x03: return 4;
	case 0x04: return 16;
	case 0x05: return 8;
	}

	return 4;
}

PakInfo::PakInfo()
: flags_(), rombanks_(), crc_()
{
	std::memset(h144x_, 0 , sizeof h144x_);
}

PakInfo::PakInfo(bool mbc1m, bool m161, bool mmm01, bool wisdomtree, unsigned rombanks, unsigned crc, unsigned char const romheader[])
: flags_(  mbc1m * flag_mbc1m
         + isHeaderChecksumOk(romheader) * flag_header_checksum_ok
         + m161 * flag_m161
         + mmm01 * flag_mmm01
         + wisdomtree * flag_wisdom_tree),
  rombanks_(rombanks), crc_(crc)
{
	std::memcpy(h144x_, romheader + 0x144, sizeof h144x_);
}

bool PakInfo::headerChecksumOk() const { return flags_ & flag_header_checksum_ok; }

static char const * h147ToCstr(unsigned char const h147, bool const mbc1m, bool const m161, bool const mmm01, bool const wisdomtree) {
	switch (h147) {
	case 0x00: return wisdomtree ? "Wisdom Tree" : (m161 ? "M161" : "NULL");
	case 0x01: return mbc1m ? "MBC1M" : "MBC1";
	case 0x02: return mbc1m ? "MBC1M [RAM]" : "MBC1 [RAM]";
	case 0x03: return mbc1m ? "MBC1M [RAM,battery]" : "MBC1 [RAM,battery]";
	case 0x05: return "MBC2";
	case 0x06: return "MBC2 [battery]";
	case 0x08: return "NULL [RAM]";
	case 0x09: return "NULL [RAM,battery]";
	case 0x0B: return "MMM01";
	case 0x0C: return "MMM01 [RAM]";
	case 0x0D: return "MMM01 [RAM,battery]";
	case 0x0F: return "MBC3 [RTC,battery]";
	case 0x10: return "MBC3 [RAM,RTC,battery]";
	case 0x11: return mmm01 ? "MMM01" : "MBC3";
	case 0x12: return "MBC3 [RAM]";
	case 0x13: return "MBC3 [RAM,battery]";
	case 0x19: return "MBC5";
	case 0x1A: return "MBC5 [RAM]";
	case 0x1B: return "MBC5 [RAM,battery]";
	case 0x1C: return "MBC5 [rumble]";
	case 0x1D: return "MBC5 [RAM,rumble]";
	case 0x1E: return "MBC5 [RAM,rumble,battery]";
	case 0xFC: return "Pocket Camera";
	case 0xFD: return "Bandai TAMA5";
	case 0xFE: return "HuC3";
	case 0xFF: return "HuC1 [RAM,battery]";
	}

	return "Unknown";
}

std::string const PakInfo::mbc() const { return h147ToCstr(h144x_[3], flags_ & flag_mbc1m, flags_ & flag_m161, flags_ & flag_mmm01, flags_ & flag_wisdom_tree); }
unsigned PakInfo::rambanks() const { return numRambanksFromH14x(h144x_[3], h144x_[5]); }
unsigned PakInfo::rombanks() const { return rombanks_; }
unsigned PakInfo::crc() const { return crc_; }

}
