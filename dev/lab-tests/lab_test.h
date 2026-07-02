#ifndef MACHINE_LAB_LAB_TEST_H
#define MACHINE_LAB_LAB_TEST_H

#include <stdio.h>

#define TEST_REQUIRE(expr)                                                         \
  do {                                                                             \
    if (!(expr)) {                                                                 \
      fprintf(stderr, "%s:%d: requirement failed: %s\n", __FILE__, __LINE__,      \
              #expr);                                                              \
      return 1;                                                                    \
    }                                                                              \
  } while (0)

#define TEST_CHECK(expr)                                                           \
  do {                                                                             \
    if (!(expr)) {                                                                 \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr);    \
      failures++;                                                                  \
    }                                                                              \
  } while (0)

#define TEST_DONE(name)                                                            \
  do {                                                                             \
    if (failures != 0) {                                                           \
      fprintf(stderr, "%s: %d checks failed\n", name, failures);                  \
      return 1;                                                                    \
    }                                                                              \
    printf("%s passed\n", name);                                                   \
    return 0;                                                                      \
  } while (0)

#endif
