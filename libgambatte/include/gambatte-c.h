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

/* C interface for using libgambatte */

#ifndef GAMBATTE_C_H
#define GAMBATTE_C_H

#include "config.h"

#ifndef __cplusplus
#include <stdint.h>
#include <stdbool.h>

typedef struct GB GB;
typedef struct FPtrs FPtrs;
typedef unsigned(InputGetter)(void *);
typedef void (*MemoryCallback)(int32_t address, int64_t cycleOffset);
typedef void (*CDCallback)(int32_t addr, int32_t addrtype, int32_t flags);
#else
#include "gambatte.h"
#include "newstate.h"

using GB = gambatte::GB;
using FPtrs = gambatte::FPtrs;
using InputGetter = gambatte::InputGetter;
using MemoryCallback = gambatte::MemoryCallback;
using CDCallback = gambatte::CDCallback;
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define GBEXPORT __declspec(dllexport)
#elif defined(LIBGAMBATTE_HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)
#define GBEXPORT __attribute__((visibility("default")))
#else
#define GBEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

GBEXPORT int gambatte_revision();

GBEXPORT GB *gambatte_create();

GBEXPORT void gambatte_destroy(GB *g);

GBEXPORT int gambatte_load(GB *g, char const *romfile, unsigned flags);

GBEXPORT int gambatte_loadbuf(GB *g, char const *romfiledata, unsigned romfilelength, unsigned flags);

GBEXPORT int gambatte_loadbios(GB *g, char const *biosfile, unsigned size, unsigned crc);

GBEXPORT int gambatte_loadbiosbuf(GB *g, char const *biosfiledata, unsigned size);

GBEXPORT int gambatte_runfor(GB *g, unsigned *videoBuf, int pitch, unsigned *audioBuf, unsigned *samples);

GBEXPORT int gambatte_updatescreenborder(GB *g, unsigned *videoBuf, int pitch);

GBEXPORT int gambatte_generatesgbsamples(GB *g, short *audioBuf, unsigned *samples);

GBEXPORT int gambatte_generatembcsamples(GB *g, short *audioBuf);

GBEXPORT void gambatte_setlayers(GB *g, unsigned mask);

GBEXPORT void gambatte_settimemode(GB *g, bool useCycles);

GBEXPORT void gambatte_setrtcdivisoroffset(GB *g, int rtcDivisorOffset);

GBEXPORT void gambatte_reset(GB *g, unsigned samplesToStall);

GBEXPORT void gambatte_setdmgpalettecolor(GB *g, unsigned palnum, unsigned colornum, unsigned rgb32);

GBEXPORT void gambatte_setcgbpalette(GB *g, unsigned *lut);

GBEXPORT void gambatte_setinputgetter(GB *g, InputGetter *getInput, void *p);

GBEXPORT int gambatte_getjoypadindex(GB *g);

GBEXPORT void gambatte_setreadcallback(GB *g, MemoryCallback callback);

GBEXPORT void gambatte_setwritecallback(GB *g, MemoryCallback callback);

GBEXPORT void gambatte_setexeccallback(GB *g, MemoryCallback callback);

GBEXPORT void gambatte_setcdcallback(GB *g, CDCallback cdc);

GBEXPORT void gambatte_settracecallback(GB *g, void (*callback)(void *));

GBEXPORT void gambatte_setscanlinecallback(GB *g, void (*callback)(), int sl);

GBEXPORT void gambatte_setlinkcallback(GB *g, void (*callback)());

GBEXPORT void gambatte_setcameracallback(GB *g, void (*callback)(int *cameraBuf));

GBEXPORT void gambatte_setremotecallback(GB *g, unsigned char (*callback)());

GBEXPORT void gambatte_setcartbuspulluptime(GB *g, unsigned cartBusPullUpTime);

GBEXPORT int gambatte_iscgb(GB *g);

GBEXPORT int gambatte_iscgbdmg(GB *g);

GBEXPORT int gambatte_isloaded(GB *g);

GBEXPORT void gambatte_savesavedata(GB *g, char *dest);

GBEXPORT void gambatte_loadsavedata(GB *g, char const *data);

GBEXPORT int gambatte_getsavedatalength(GB *g);

GBEXPORT int gambatte_newstatelen(GB *g);

GBEXPORT int gambatte_newstatesave(GB *g, char *data, int len);

GBEXPORT int gambatte_newstateload(GB *g, char const *data, int len);

GBEXPORT void gambatte_newstatesave_ex(GB *g, FPtrs *ff);

GBEXPORT void gambatte_newstateload_ex(GB *g, FPtrs *ff);

GBEXPORT void gambatte_romtitle(GB *g, char *dest);

GBEXPORT void gambatte_pakinfo(GB *g, char *mbc, unsigned *rambanks, unsigned *rombanks, unsigned *crc, unsigned *headerChecksumOk);

GBEXPORT int gambatte_getmemoryarea(GB *g, int which, unsigned char **data, int *length);

GBEXPORT unsigned gambatte_savestate(GB *g, unsigned const *videoBuf, int pitch, char *stateBuf);

GBEXPORT bool gambatte_loadstate(GB *g, char const *stateBuf, unsigned size);

GBEXPORT unsigned char gambatte_cpuread(GB *g, unsigned short addr);

GBEXPORT void gambatte_cpuwrite(GB *g, unsigned short addr, unsigned char val);

GBEXPORT int gambatte_linkstatus(GB *g, int which);

GBEXPORT unsigned gambatte_getbank(GB *g, unsigned type);

GBEXPORT unsigned gambatte_getaddrbank(GB *g, unsigned short addr);

GBEXPORT void gambatte_setbank(GB *g, unsigned type, unsigned bank);

GBEXPORT void gambatte_setaddrbank(GB *g, unsigned short addr, unsigned bank);

GBEXPORT void gambatte_getregs(GB *g, int *dest);

GBEXPORT void gambatte_setregs(GB *g, int *src);

GBEXPORT void gambatte_setinterruptaddresses(GB *g, int *addrs, int numAddrs);

GBEXPORT int gambatte_gethitinterruptaddress(GB *g);

GBEXPORT unsigned long long gambatte_timenow(GB *g);

GBEXPORT void gambatte_settime(GB *g, unsigned long long dividers);

GBEXPORT int gambatte_getdivstate(GB *g);

GBEXPORT void gambatte_setspeedupflags(GB *g, unsigned flags);

#ifdef __cplusplus
}
#endif

#endif /* GAMBATTE_C_H */
