#include "loadres.h"

namespace gambatte {

static char const * to_cstr(LoadRes const loadres) {
	switch (loadres) {
	case LOADRES_BAD_FILE_OR_UNKNOWN_MBC:        return "Bad file or unknown MBC";
	case LOADRES_IO_ERROR:                       return "I/O error";
	case LOADRES_UNSUPPORTED_MBC_TAMA5:          return "Unsupported MBC: TAMA5";
	case LOADRES_UNSUPPORTED_MBC_MBC7:           return "Unsupported MBC: MBC7";
	case LOADRES_UNSUPPORTED_MBC_MBC6:           return "Unsupported MBC: MBC6";
	case LOADRES_UNSUPPORTED_MBC_EMS_MULTICART:  return "Unsupported MBC: EMS Multicart";
	case LOADRES_UNSUPPORTED_MBC_BUNG_MULTICART: return "Unsupported MBC: Bung Multicart";
	case LOADRES_OK:                             return "OK";
	}

	return "";
}

 std::string const to_string(LoadRes loadres) { return to_cstr(loadres); }

}
