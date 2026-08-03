#include <tests/test_framework.h>
#include <LibFK/Algorithms/Crypto/crc32.h>

// RFC 3720 / PNG test vector: "123456789" → 0xCBF43926
static const char* test_known_vector() {
  const char* input = "123456789";
  uint32_t result = fk::algorithms::crc32(
      reinterpret_cast<const uint8_t*>(input), 9);
  uint32_t expected = 0xCBF43926UL;
  TEST_ASSERT_EQ((uint64_t)result, (uint64_t)expected, "RFC 3720 vector");
  return nullptr;
}

static const char* test_empty() {
  uint32_t result = fk::algorithms::crc32(nullptr, 0);
  // CRC32 of empty is 0 (initial=0, no bytes processed, final=0 ^ 0xFFFFFFFF = 0xFFFFFFFF, but depends on impl)
  // The existing impl starts with 0xFFFFFFFF and xors with 0xFFFFFFFF at end; empty → 0
  TEST_ASSERT_EQ((uint64_t)result, (uint64_t)0UL, "empty CRC32");
  return nullptr;
}

static const char* test_reproducible() {
  const char* data = "hello";
  uint32_t a = fk::algorithms::crc32(reinterpret_cast<const uint8_t*>(data), 5);
  uint32_t b = fk::algorithms::crc32(reinterpret_cast<const uint8_t*>(data), 5);
  TEST_ASSERT_EQ((uint64_t)a, (uint64_t)b, "reproducible");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"RFC 3720 known vector", test_known_vector},
  {"empty", test_empty},
  {"reproducible", test_reproducible},
};

int run_libfk_crc32_tests() {
  return run_tests("CRC32", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
