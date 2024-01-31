#ifndef CRC32_H
#define CRC32_H

#ifdef HAVE_ZLIB_H
#include <zlib.h>
#else

#include <cstring>
#include "gbint.h"

namespace gambatte {

gambatte::uint_least32_t crc32(gambatte::uint_least32_t crc, void const *buf, std::size_t size);

}

#endif

#endif
