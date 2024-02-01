#ifndef FPTRS_H
#define FPTRS_H

#include <stddef.h> /* size_t */

#ifdef __cplusplus
namespace gambatte {
extern "C" {
#endif
typedef struct FPtrs {
  void (*Save_)(void const *ptr, size_t size, char const *name);
  void (*Load_)(void *ptr, size_t size, char const *name);
  void (*EnterSection_)(char const *name);
  void (*ExitSection_)(char const *name);
} FPtrs;
#ifdef __cplusplus
}
}
#endif

#endif /* FPTRS_H */
