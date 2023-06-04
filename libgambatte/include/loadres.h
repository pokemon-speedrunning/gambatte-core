#ifndef GAMBATTE_LOADRES_H
#define GAMBATTE_LOADRES_H

#include "gbexport.h"

#include <string>

namespace gambatte {

enum LoadRes {
	LOADRES_BAD_FILE_OR_UNKNOWN_MBC = -0x7FFF,
	LOADRES_IO_ERROR,
	LOADRES_UNSUPPORTED_MBC_TAMA5,
	LOADRES_UNSUPPORTED_MBC_MBC7 = -0x122,
	LOADRES_UNSUPPORTED_MBC_MBC6 = -0x120,
	LOADRES_UNSUPPORTED_MBC_EMS_MULTICART,
	LOADRES_UNSUPPORTED_MBC_BUNG_MULTICART,
	LOADRES_OK = 0
};

GBEXPORT std::string const to_string(LoadRes);

}

#endif
