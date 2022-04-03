#ifndef PAKINFO_INTERNAL_H
#define PAKINFO_INTERNAL_H

#include "pakinfo.h"

namespace gambatte {

bool isHeaderChecksumOk(unsigned const char header[]);
unsigned numRambanksFromH14x(unsigned char h147, unsigned char h149);

}

#endif
