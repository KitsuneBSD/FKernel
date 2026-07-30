#include <tests/test_framework.h>
#include <LibFK/Algorithms/byte_order.h>

static const char* test_htons_round_trip() {
  uint16_t v = 0x1234;
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::ntohs(fk::algorithms::htons(v)), (uint64_t)v, "htons/ntohs round trip");
  return nullptr;
}

static const char* test_htonl_round_trip() {
  uint32_t v = 0x12345678UL;
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::ntohl(fk::algorithms::htonl(v)), (uint64_t)v, "htonl/ntohl round trip");
  return nullptr;
}

static const char* test_zero() {
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::htons(0), (uint64_t)0, "htons(0)");
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::htonl(0), (uint64_t)0, "htonl(0)");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"htons/ntohs round trip", test_htons_round_trip},
  {"htonl/ntohl round trip", test_htonl_round_trip},
  {"zero", test_zero},
};

int run_libfk_byte_order_tests() {
  return run_tests("ByteOrder", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
