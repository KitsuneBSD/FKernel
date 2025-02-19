#include "../../Include/LibK/stdint.h"
#include "./Include/tests.h"
#include <assert.h>

void test_macros(void) {
  int8_t a = INT8_C(42);
  ASSERT_EQ(a, 42);

  uint8_t b = UINT8_C(42);
  ASSERT_EQ(b, 42);

  int16_t c = INT16_C(1000);
  ASSERT_EQ(c, 1000);

  uint16_t d = UINT16_C(1000);
  ASSERT_EQ(d, 1000);

  int32_t e = INT32_C(100000);
  ASSERT_EQ(e, 100000);

  uint32_t f = UINT32_C(100000);
  ASSERT_EQ(f, 100000);

  int64_t g = INT64_C(10000000000);
  ASSERT_EQ(g, 10000000000LL);

  uint64_t h = UINT64_C(10000000000);
  ASSERT_EQ(h, 10000000000ULL);
}

void test_typedefs(void) {
  _Static_assert(sizeof(int8_t) == 1, "int8_t must be 1 byte");
  _Static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
  _Static_assert(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
  _Static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
  _Static_assert(sizeof(int32_t) == 4, "int32_t must be 4 bytes");
  _Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
  _Static_assert(sizeof(int64_t) == 8, "int64_t must be 8 bytes");
  _Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");

  _Static_assert(sizeof(int_least8_t) >= 1,
                 "int_least8_t size must be >= 1 byte");
  _Static_assert(sizeof(uint_least8_t) >= 1,
                 "uint_least8_t size must be >= 1 byte");
  _Static_assert(sizeof(int_least16_t) >= 2,
                 "int_least16_t size must be >= 2 bytes");
  _Static_assert(sizeof(uint_least16_t) >= 2,
                 "uint_least16_t size must be >= 2 bytes");
  _Static_assert(sizeof(int_least32_t) >= 4,
                 "int_least32_t size must be >= 4 bytes");
  _Static_assert(sizeof(uint_least32_t) >= 4,
                 "uint_least32_t size must be >= 4 bytes");
  _Static_assert(sizeof(int_least64_t) >= 8,
                 "int_least64_t size must be >= 8 bytes");
  _Static_assert(sizeof(uint_least64_t) >= 8,
                 "uint_least64_t size must be >= 8 bytes");

  _Static_assert(sizeof(int_fast8_t) >= 1,
                 "int_fast8_t size must be >= 1 byte");
  _Static_assert(sizeof(uint_fast8_t) >= 1,
                 "uint_fast8_t size must be >= 1 byte");
  _Static_assert(sizeof(int_fast16_t) >= 2,
                 "int_fast16_t size must be >= 2 bytes");
  _Static_assert(sizeof(uint_fast16_t) >= 2,
                 "uint_fast16_t size must be >= 2 bytes");
  _Static_assert(sizeof(int_fast32_t) >= 4,
                 "int_fast32_t size must be >= 4 bytes");
  _Static_assert(sizeof(uint_fast32_t) >= 4,
                 "uint_fast32_t size must be >= 4 bytes");
  _Static_assert(sizeof(int_fast64_t) >= 8,
                 "int_fast64_t size must be >= 8 bytes");
  _Static_assert(sizeof(uint_fast64_t) >= 8,
                 "uint_fast64_t size must be >= 8 bytes");
}

int main(void) {
  RUN_TEST(test_macros);
  RUN_TEST(test_typedefs);

  print_test_summary();
  return tests_failed > 0 ? 1 : 0;
}
