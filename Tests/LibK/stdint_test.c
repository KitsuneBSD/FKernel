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

void test_limits(void) {
  ASSERT_EQ(INT8_MIN, -128);
  ASSERT_EQ(INT8_MAX, 127);
  ASSERT_EQ(UINT8_MAX, 255);

  ASSERT_EQ(INT16_MIN, -32768);
  ASSERT_EQ(INT16_MAX, 32767);
  ASSERT_EQ(UINT16_MAX, 65535);

  ASSERT_EQ(INT32_MIN, -2147483648);
  ASSERT_EQ(INT32_MAX, 2147483647);
  ASSERT_EQ(UINT32_MAX, 4294967295U);

  ASSERT_EQ(INT64_MIN, (-9223372036854775807LL - 1));
  ASSERT_EQ(INT64_MAX, 9223372036854775807LL);
  ASSERT_EQ(UINT64_MAX, 18446744073709551615ULL);

  ASSERT_EQ(INT_LEAST8_MIN, INT8_MIN);
  ASSERT_EQ(INT_LEAST8_MAX, INT8_MAX);
  ASSERT_EQ(UINT_LEAST8_MAX, UINT8_MAX);

  ASSERT_EQ(INT_LEAST16_MIN, INT16_MIN);
  ASSERT_EQ(INT_LEAST16_MAX, INT16_MAX);
  ASSERT_EQ(UINT_LEAST16_MAX, UINT16_MAX);

  ASSERT_EQ(INT_LEAST32_MIN, INT32_MIN);
  ASSERT_EQ(INT_LEAST32_MAX, INT32_MAX);
  ASSERT_EQ(UINT_LEAST32_MAX, UINT32_MAX);

  ASSERT_EQ(INT_LEAST64_MIN, INT64_MIN);
  ASSERT_EQ(INT_LEAST64_MAX, INT64_MAX);
  ASSERT_EQ(UINT_LEAST64_MAX, UINT64_MAX);

  ASSERT_EQ(INT_FAST8_MIN, INT8_MIN);
  ASSERT_EQ(INT_FAST8_MAX, INT8_MAX);
  ASSERT_EQ(UINT_FAST8_MAX, UINT8_MAX);

  ASSERT_EQ(INT_FAST16_MIN, INT32_MIN);
  ASSERT_EQ(INT_FAST16_MAX, INT32_MAX);
  ASSERT_EQ(UINT_FAST16_MAX, UINT32_MAX);

  ASSERT_EQ(INT_FAST32_MIN, INT32_MIN);
  ASSERT_EQ(INT_FAST32_MAX, INT32_MAX);
  ASSERT_EQ(UINT_FAST32_MAX, UINT32_MAX);

  ASSERT_EQ(INT_FAST64_MIN, INT64_MIN);
  ASSERT_EQ(INT_FAST64_MAX, INT64_MAX);
  ASSERT_EQ(UINT_FAST64_MAX, UINT64_MAX);
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
  RUN_TEST(test_limits);
  RUN_TEST(test_typedefs);

  print_test_summary();
  return tests_failed > 0 ? 1 : 0;
}
