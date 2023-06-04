#include "gambatte-c.h"

#include <stdlib.h>

/* Simple test to ensure the C api can be used from C */

int main() {
  GB *test_gb = gambatte_create();
  if (test_gb == NULL) {
    return EXIT_FAILURE;
  }
  gambatte_destroy(test_gb);
  return EXIT_SUCCESS;
}
