#ifndef GBCALLBACK_H
#define GBCALLBACK_H

#include <stdint.h>

#ifdef __cplusplus
namespace gambatte {
extern "C" {
#endif

typedef void (*MemoryCallback)(int32_t address, int64_t cycleOffset);
typedef void (*CDCallback)(int32_t addr, int32_t addrtype, int32_t flags);

#ifdef __cplusplus
}
}
#endif

#endif /* GBCALLBACK_H */
