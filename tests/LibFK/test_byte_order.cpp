#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/byte_order.h>

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

static const char* test_swap16() {
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::swap16(0x1234), (uint64_t)0x3412, "swap16");
  return nullptr;
}

static const char* test_swap32() {
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::swap32(0x12345678UL), (uint64_t)0x78563412UL, "swap32");
  return nullptr;
}

static const char* test_swap64() {
  uint64_t v = 0x1122334455667788ULL;
  uint64_t s = fk::algorithms::swap64(v);
  TEST_ASSERT_EQ(s, (uint64_t)0x8877665544332211ULL, "swap64");
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::swap64(s), v, "swap64 round trip");
  return nullptr;
}

static const char* test_htonll_round_trip() {
  uint64_t v = 0x1122334455667788ULL;
  TEST_ASSERT_EQ(fk::algorithms::ntohll(fk::algorithms::htonll(v)), v, "htonll/ntohll round trip");
  return nullptr;
}

static const char* test_load_le16() {
  uint8_t buf[] = {0x34, 0x12};
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::load_le16(buf, 0), (uint64_t)0x1234, "load_le16");
  return nullptr;
}

static const char* test_load_le32() {
  uint8_t buf[] = {0x78, 0x56, 0x34, 0x12};
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::load_le32(buf, 0), (uint64_t)0x12345678UL, "load_le32");
  return nullptr;
}

static const char* test_load_le64() {
  uint8_t buf[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
  TEST_ASSERT_EQ(fk::algorithms::load_le64(buf, 0), (uint64_t)0x1122334455667788ULL, "load_le64");
  return nullptr;
}

static const char* test_load_be16() {
  uint8_t buf[] = {0x12, 0x34};
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::load_be16(buf, 0), (uint64_t)0x1234, "load_be16");
  return nullptr;
}

static const char* test_load_be32() {
  uint8_t buf[] = {0x12, 0x34, 0x56, 0x78};
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::load_be32(buf, 0), (uint64_t)0x12345678UL, "load_be32");
  return nullptr;
}

static const char* test_load_be64() {
  uint8_t buf[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
  TEST_ASSERT_EQ(fk::algorithms::load_be64(buf, 0), (uint64_t)0x1122334455667788ULL, "load_be64");
  return nullptr;
}

static const char* test_load_offset() {
  uint8_t buf[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x34, 0x12};
  TEST_ASSERT_EQ((uint64_t)fk::algorithms::load_le16(buf, 4), (uint64_t)0x1234, "load_le16 with offset");
  return nullptr;
}

static const char* test_swap64_zero() {
  TEST_ASSERT_EQ(fk::algorithms::swap64(0), (uint64_t)0, "swap64(0)");
  TEST_ASSERT_EQ(fk::algorithms::swap64(0xFFFFFFFFFFFFFFFFULL), (uint64_t)0xFFFFFFFFFFFFFFFFULL, "swap64(all ones)");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"htons/ntohs round trip", test_htons_round_trip},
  {"htonl/ntohl round trip", test_htonl_round_trip},
  {"zero", test_zero},
  {"swap16", test_swap16},
  {"swap32", test_swap32},
  {"swap64", test_swap64},
  {"htonll/ntohll round trip", test_htonll_round_trip},
  {"load_le16", test_load_le16},
  {"load_le32", test_load_le32},
  {"load_le64", test_load_le64},
  {"load_be16", test_load_be16},
  {"load_be32", test_load_be32},
  {"load_be64", test_load_be64},
  {"load with offset", test_load_offset},
  {"swap64 zero/ones", test_swap64_zero},
};

int run_libfk_byte_order_tests() {
  return run_tests("ByteOrder", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
