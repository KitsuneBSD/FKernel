#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/bitmap.h>

using fk::algorithms::test_bit;
using fk::algorithms::set_bit;
using fk::algorithms::clear_bit;
using fk::algorithms::find_first_free_bit;

static const char* test_set_and_test() {
    uint8_t bm[4] = {0};
    set_bit(bm, 0);
    set_bit(bm, 7);
    set_bit(bm, 15);
    set_bit(bm, 31);
    TEST_ASSERT(test_bit(bm, 0), "bit 0 set");
    TEST_ASSERT(test_bit(bm, 7), "bit 7 set");
    TEST_ASSERT(test_bit(bm, 15), "bit 15 set");
    TEST_ASSERT(test_bit(bm, 31), "bit 31 set");
    TEST_ASSERT(!test_bit(bm, 1), "bit 1 clear");
    TEST_ASSERT(!test_bit(bm, 8), "bit 8 clear");
    TEST_ASSERT(!test_bit(bm, 30), "bit 30 clear");
    TEST_ASSERT_EQ((int)bm[0], (int)0x81, "byte 0 pattern");
    TEST_ASSERT_EQ((int)bm[1], (int)0x80, "byte 1 pattern");
    return nullptr;
}

static const char* test_clear_bit() {
    uint8_t bm[2] = {0xFF, 0xFF};
    clear_bit(bm, 3);
    clear_bit(bm, 12);
    TEST_ASSERT(!test_bit(bm, 3), "bit 3 cleared");
    TEST_ASSERT(!test_bit(bm, 12), "bit 12 cleared");
    TEST_ASSERT(test_bit(bm, 2), "bit 2 intact");
    TEST_ASSERT(test_bit(bm, 13), "bit 13 intact");
    TEST_ASSERT_EQ((int)bm[0], (int)0xF7, "byte 0 after clear");
    TEST_ASSERT_EQ((int)bm[1], (int)0xEF, "byte 1 after clear");
    return nullptr;
}

static const char* test_find_first_free() {
    uint8_t bm[3] = {0xFF, 0x00, 0xAA};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 24), 8L, "first free is bit 8");
    return nullptr;
}

static const char* test_find_first_free_partial() {
    uint8_t bm[3] = {0xFE, 0x00, 0x00};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 24), 0L, "first free is bit 0");
    return nullptr;
}

static const char* test_find_first_free_middle_bit() {
    uint8_t bm[2] = {0x8F, 0x00};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16), 4L, "first free is bit 4");
    return nullptr;
}

static const char* test_find_first_free_skips_full() {
    uint8_t bm[2] = {0xFF, 0x0F};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16), 12L, "skips full byte 0");
    return nullptr;
}

static const char* test_find_first_free_all_full() {
    uint8_t bm[2] = {0xFF, 0xFF};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16), 16L, "all full returns nbits");
    return nullptr;
}

static const char* test_find_first_free_nbits_boundary() {
    uint8_t bm[2] = {0x00, 0x00};
    // nbits = 10 → only bits 0..9 are scanned; bit 10 is out of range
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 10), 0L, "first free within range");
    return nullptr;
}

static const char* test_find_first_free_with_start() {
    uint8_t bm[2] = {0x0F, 0x00};
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16, 4), 4L, "start at boundary bit");
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16, 5), 5L, "start mid-byte");
    TEST_ASSERT_EQ((long)find_first_free_bit(bm, 16, 4 + 8), 12L, "start after byte 0");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"set and test", test_set_and_test},
    {"clear bit", test_clear_bit},
    {"find first free", test_find_first_free},
    {"find first free partial", test_find_first_free_partial},
    {"find first free middle bit", test_find_first_free_middle_bit},
    {"find first free skips full", test_find_first_free_skips_full},
    {"find first free all full", test_find_first_free_all_full},
    {"find first free nbits boundary", test_find_first_free_nbits_boundary},
    {"find first free with start", test_find_first_free_with_start},
};

int run_libfk_bitmap_tests() {
    return run_tests("Bitmap", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
