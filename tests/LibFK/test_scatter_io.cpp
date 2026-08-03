#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/scatter_io.h>

using fk::core::Error;

namespace {

// Fake "disk" of 5 blocks of 16 bytes. Physical blocks 1..4 hold data
// (block 0 is the sparse-hole sentinel). Logical block L maps to phys L+1,
// and logical block 1 is sparse (maps to phys 0).
uint8_t s_disk[80];

constexpr size_t BLOCK = 16;
constexpr size_t phys_of(uint32_t lblock) { return (lblock + 1) * BLOCK; }

void init_disk() {
    for (size_t i = 0; i < sizeof(s_disk); ++i) s_disk[i] = static_cast<uint8_t>(i);
}

fk::core::Result<uint64_t, Error> resolve_block(uint32_t lblock) {
    if (lblock == 1) return static_cast<uint64_t>(0); // sparse hole
    if (lblock >= 4) return Error::InvalidParameter;
    return static_cast<uint64_t>(lblock + 1); // phys blocks 1..4
}

fk::core::Result<void, Error> read_block(uint64_t phys, uint8_t* scratch) {
    if (phys == 0 || phys > 4) return Error::InvalidParameter;
    for (size_t i = 0; i < BLOCK; ++i) scratch[i] = s_disk[phys * BLOCK + i];
    return {};
}

fk::core::Result<void, Error> write_block(uint64_t phys, const uint8_t* scratch) {
    if (phys == 0 || phys > 4) return Error::InvalidParameter;
    for (size_t i = 0; i < BLOCK; ++i) s_disk[phys * BLOCK + i] = scratch[i];
    return {};
}

} // namespace

static const char* test_read_aligned_full_block() {
    init_disk();
    uint8_t buf[BLOCK];
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_read(0, BLOCK, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_ok(), "aligned read should succeed");
    TEST_ASSERT_EQ(16, (int)res.value(), "should read 16 bytes");
    TEST_ASSERT(__builtin_memcmp(buf, s_disk + phys_of(0), BLOCK) == 0, "aligned read should match disk");
    return nullptr;
}

static const char* test_read_partial_spanning() {
    init_disk();
    // Offset 8, 32 bytes: 8B block0 + 16B sparse block1 + 8B block2
    uint8_t buf[32];
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_read(8, 32, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_ok(), "spanning read should succeed");
    TEST_ASSERT_EQ(32, (int)res.value(), "should read 32 bytes");
    for (int i = 0; i < 8; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)s_disk[phys_of(0) + 8 + i], "head slice");
    for (int i = 8; i < 24; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)0, "sparse slice zero");
    for (int i = 24; i < 32; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)s_disk[phys_of(2) + (i - 24)], "tail slice");
    return nullptr;
}

static const char* test_read_sparse_zero_fill() {
    init_disk();
    uint8_t buf[BLOCK];
    uint8_t scratch[BLOCK];
    // Block 1 (offset 16) is sparse
    auto res = fk::algorithms::scatter_read(16, BLOCK, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_ok(), "sparse read should succeed");
    for (int i = 0; i < (int)BLOCK; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)0, "sparse block should be zero-filled");
    return nullptr;
}

static const char* test_read_partial_sparse() {
    init_disk();
    uint8_t buf[24];
    uint8_t scratch[BLOCK];
    // Offset 12, 24 bytes: 4B block0 + 16B sparse block1 + 4B block2
    auto res = fk::algorithms::scatter_read(12, 24, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_ok(), "partial sparse read should succeed");
    TEST_ASSERT_EQ(24, (int)res.value(), "should read 24 bytes");
    for (int i = 0; i < 4; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)s_disk[phys_of(0) + 12 + i], "head slice");
    for (int i = 4; i < 20; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)0, "sparse slice zero");
    for (int i = 20; i < 24; ++i)
        TEST_ASSERT_EQ((uint64_t)buf[i], (uint64_t)s_disk[phys_of(2) + (i - 20)], "tail slice");
    return nullptr;
}

static const char* test_read_zero_size() {
    uint8_t buf[4] = {1, 2, 3, 4};
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_read(0, 0, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_ok(), "zero-size read should succeed");
    TEST_ASSERT_EQ(0, (int)res.value(), "zero-size read returns 0");
    TEST_ASSERT_EQ((uint64_t)buf[0], (uint64_t)1, "buffer untouched");
    return nullptr;
}

static const char* test_read_error_propagated() {
    uint8_t buf[BLOCK];
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_read(64, BLOCK, buf, BLOCK, scratch, resolve_block, read_block);
    TEST_ASSERT(res.is_error(), "out-of-range block should error");
    return nullptr;
}

static const char* test_write_full_block() {
    init_disk();
    uint8_t data[BLOCK];
    for (int i = 0; i < (int)BLOCK; ++i) data[i] = static_cast<uint8_t>(0xA0 + i);
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_write(0, BLOCK, data, BLOCK, scratch, resolve_block, read_block, write_block);
    TEST_ASSERT(res.is_ok(), "full-block write should succeed");
    TEST_ASSERT(__builtin_memcmp(s_disk + phys_of(0), data, BLOCK) == 0, "full-block write data");
    return nullptr;
}

static const char* test_write_partial_rmw() {
    init_disk();
    // Write 4 bytes at offset 2 of block 0: rest of block must be preserved
    uint8_t data[4] = {0xEE, 0xEE, 0xEE, 0xEE};
    uint8_t scratch[BLOCK];
    auto res = fk::algorithms::scatter_write(2, 4, data, BLOCK, scratch, resolve_block, read_block, write_block);
    TEST_ASSERT(res.is_ok(), "partial write should succeed");
    TEST_ASSERT_EQ((uint64_t)s_disk[phys_of(0) + 0], (uint64_t)(phys_of(0) + 0), "byte 0 preserved");
    TEST_ASSERT_EQ((uint64_t)s_disk[phys_of(0) + 1], (uint64_t)(phys_of(0) + 1), "byte 1 preserved");
    TEST_ASSERT_EQ((uint64_t)s_disk[phys_of(0) + 2], (uint64_t)0xEE, "byte 2 overwritten");
    TEST_ASSERT_EQ((uint64_t)s_disk[phys_of(0) + 5], (uint64_t)0xEE, "byte 5 overwritten");
    TEST_ASSERT_EQ((uint64_t)s_disk[phys_of(0) + 6], (uint64_t)(phys_of(0) + 6), "byte 6 preserved");
    return nullptr;
}

static const char* test_write_spanning() {
    init_disk();
    uint8_t data[20];
    for (int i = 0; i < 20; ++i) data[i] = static_cast<uint8_t>(0x10 + i);
    uint8_t scratch[BLOCK];
    // Write 20 bytes at offset 12 (block0) → crosses into block1 (sparse!)
    auto res = fk::algorithms::scatter_write(12, 20, data, BLOCK, scratch, resolve_block, read_block, write_block);
    TEST_ASSERT(res.is_error(), "write into sparse block should error");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"read aligned full block", test_read_aligned_full_block},
    {"read partial spanning", test_read_partial_spanning},
    {"read sparse zero-fill", test_read_sparse_zero_fill},
    {"read partial sparse", test_read_partial_sparse},
    {"read zero size", test_read_zero_size},
    {"read error propagated", test_read_error_propagated},
    {"write full block", test_write_full_block},
    {"write partial read-modify-write", test_write_partial_rmw},
    {"write into sparse errors", test_write_spanning},
};

int run_libfk_scatter_io_tests() {
    return run_tests("ScatterIO", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
