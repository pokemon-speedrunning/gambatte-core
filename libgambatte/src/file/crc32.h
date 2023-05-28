#ifndef CRC32_H
#define CRC32_H

#include "config.h"

#ifdef LIBGAMBATTE_SUPPORT_ZIP
#include <zlib.h>
#else

#include <cstring>

namespace gambatte {

unsigned long crc32(unsigned long crc, void const *buf, std::size_t size);

}

#endif

#endif
