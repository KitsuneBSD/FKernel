#include <tests/test_framework.h>
#include <LibFK/Algorithms/Crypto/byte_checksum.h>

static const char* test_all_zeros() {
  uint8_t data[16] = {};
  TEST_ASSERT(fk::algorithms::byte_checksum_valid(data, 16), "all zeros valid");
  return nullptr;
}

static const char* test_non_zero() {
  uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};
  TEST_ASSERT(!fk::algorithms::byte_checksum_valid(data, 4), "non-zero invalid");
  return nullptr;
}

static const char* test_round_trip() {
  uint8_t data[32];
  for (int i = 0; i < 31; ++i) data[i] = (uint8_t)(i + 1);
  data[31] = fk::algorithms::byte_checksum_for(data, 31);
  TEST_ASSERT(fk::algorithms::byte_checksum_valid(data, 32), "round trip valid");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"all zeros valid", test_all_zeros},
  {"non-zero invalid", test_non_zero},
  {"round trip", test_round_trip},
};

int run_libfk_byte_checksum_tests() {
  return run_tests("ByteChecksum", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
