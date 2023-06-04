//
//   Copyright (C) 2023 by Pierre Wendling <pierre.wendling.4@gmail.com>
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

/* C interface for using a shared libgambatte */

#ifndef GAMBATTE_C_H
#define GAMBATTE_C_H

#include "config.h"

#include "gambatte.h"
#include "newstate.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#define GBEXPORT __declspec(dllexport)
#elif defined(LIBGAMBATTE_HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)
#define GBEXPORT __attribute__((visibility("default")))
#else
#define GBEXPORT
#endif

extern "C" {

GBEXPORT int gambatte_revision();

GBEXPORT gambatte::GB *gambatte_create();

GBEXPORT void gambatte_destroy(gambatte::GB *g);

GBEXPORT int gambatte_load(gambatte::GB *g, char const *romfile, unsigned flags);

GBEXPORT int gambatte_loadbuf(gambatte::GB *g, char const *romfiledata, unsigned romfilelength, unsigned flags);

GBEXPORT int gambatte_loadbios(gambatte::GB *g, char const *biosfile, unsigned size, unsigned crc);

GBEXPORT int gambatte_loadbiosbuf(gambatte::GB *g, char const *biosfiledata, unsigned size);

GBEXPORT int gambatte_runfor(gambatte::GB *g, unsigned *videoBuf, int pitch, unsigned *audioBuf, unsigned *samples);

GBEXPORT int gambatte_updatescreenborder(gambatte::GB *g, unsigned *videoBuf, int pitch);

GBEXPORT int gambatte_generatesgbsamples(gambatte::GB *g, short *audioBuf, unsigned *samples);

GBEXPORT int gambatte_generatembcsamples(gambatte::GB *g, short *audioBuf);

GBEXPORT void gambatte_setlayers(gambatte::GB *g, unsigned mask);

GBEXPORT void gambatte_settimemode(gambatte::GB *g, bool useCycles);

GBEXPORT void gambatte_setrtcdivisoroffset(gambatte::GB *g, int rtcDivisorOffset);

GBEXPORT void gambatte_reset(gambatte::GB *g, unsigned samplesToStall);

GBEXPORT void gambatte_setdmgpalettecolor(gambatte::GB *g, unsigned palnum, unsigned colornum, unsigned rgb32);

GBEXPORT void gambatte_setcgbpalette(gambatte::GB *g, unsigned *lut);

GBEXPORT void gambatte_setinputgetter(gambatte::GB *g, gambatte::InputGetter *getInput, void *p);

GBEXPORT int gambatte_getjoypadindex(gambatte::GB *g);

GBEXPORT void gambatte_setreadcallback(gambatte::GB *g, gambatte::MemoryCallback callback);

GBEXPORT void gambatte_setwritecallback(gambatte::GB *g, gambatte::MemoryCallback callback);

GBEXPORT void gambatte_setexeccallback(gambatte::GB *g, gambatte::MemoryCallback callback);

GBEXPORT void gambatte_setcdcallback(gambatte::GB *g, gambatte::CDCallback cdc);

GBEXPORT void gambatte_settracecallback(gambatte::GB *g, void (*callback)(void *));

GBEXPORT void gambatte_setscanlinecallback(gambatte::GB *g, void (*callback)(), int sl);

GBEXPORT void gambatte_setlinkcallback(gambatte::GB *g, void (*callback)());

GBEXPORT void gambatte_setcameracallback(gambatte::GB *g, void (*callback)(int *cameraBuf));

GBEXPORT void gambatte_setremotecallback(gambatte::GB *g, unsigned char (*callback)());

GBEXPORT void gambatte_setcartbuspulluptime(gambatte::GB *g, unsigned cartBusPullUpTime);

GBEXPORT int gambatte_iscgb(gambatte::GB *g);

GBEXPORT int gambatte_iscgbdmg(gambatte::GB *g);

GBEXPORT int gambatte_isloaded(gambatte::GB *g);

GBEXPORT void gambatte_savesavedata(gambatte::GB *g, char *dest);

GBEXPORT void gambatte_loadsavedata(gambatte::GB *g, char const *data);

GBEXPORT int gambatte_getsavedatalength(gambatte::GB *g);

GBEXPORT int gambatte_newstatelen(gambatte::GB *g);

GBEXPORT int gambatte_newstatesave(gambatte::GB *g, char *data, int len);

GBEXPORT int gambatte_newstateload(gambatte::GB *g, char const *data, int len);

GBEXPORT void gambatte_newstatesave_ex(gambatte::GB *g, gambatte::FPtrs *ff);

GBEXPORT void gambatte_newstateload_ex(gambatte::GB *g, gambatte::FPtrs *ff);

GBEXPORT void gambatte_romtitle(gambatte::GB *g, char *dest);

GBEXPORT void gambatte_pakinfo(gambatte::GB *g, char *mbc, unsigned *rambanks, unsigned *rombanks, unsigned *crc, unsigned *headerChecksumOk);

GBEXPORT int gambatte_getmemoryarea(gambatte::GB *g, int which, unsigned char **data, int *length);

GBEXPORT unsigned gambatte_savestate(gambatte::GB *g, unsigned const *videoBuf, int pitch, char *stateBuf);

GBEXPORT bool gambatte_loadstate(gambatte::GB *g, char const *stateBuf, unsigned size);

GBEXPORT unsigned char gambatte_cpuread(gambatte::GB *g, unsigned short addr);

GBEXPORT void gambatte_cpuwrite(gambatte::GB *g, unsigned short addr, unsigned char val);

GBEXPORT int gambatte_linkstatus(gambatte::GB *g, int which);

GBEXPORT unsigned gambatte_getbank(gambatte::GB *g, unsigned type);

GBEXPORT unsigned gambatte_getaddrbank(gambatte::GB *g, unsigned short addr);

GBEXPORT void gambatte_setbank(gambatte::GB *g, unsigned type, unsigned bank);

GBEXPORT void gambatte_setaddrbank(gambatte::GB *g, unsigned short addr, unsigned bank);

GBEXPORT void gambatte_getregs(gambatte::GB *g, int *dest);

GBEXPORT void gambatte_setregs(gambatte::GB *g, int *src);

GBEXPORT void gambatte_setinterruptaddresses(gambatte::GB *g, int *addrs, int numAddrs);

GBEXPORT int gambatte_gethitinterruptaddress(gambatte::GB *g);

GBEXPORT unsigned long long gambatte_timenow(gambatte::GB *g);

GBEXPORT void gambatte_settime(gambatte::GB *g, unsigned long long dividers);

GBEXPORT int gambatte_getdivstate(gambatte::GB *g);

GBEXPORT void gambatte_setspeedupflags(gambatte::GB *g, unsigned flags);
}

#endif /* GAMBATTE_C_H */
