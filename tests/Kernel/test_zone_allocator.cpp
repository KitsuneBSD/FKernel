#include <tests/test_framework.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_allocator.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_defs.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_types.h>

// Tests for Zone (populate/accessors) and classify_zone / zone_limit helpers.
// All deps are LibFK/LibC — no kernel hardware required.

/* ---- classify_zone ---- */

static const char* test_classify_dma() {
    TEST_ASSERT_EQ(ZoneType::DMA, classify_zone(0), "0 is DMA");
    TEST_ASSERT_EQ(ZoneType::DMA, classify_zone(DMA_LIMIT - 1), "just below DMA_LIMIT is DMA");
    return nullptr;
}

static const char* test_classify_normal() {
    TEST_ASSERT_EQ(ZoneType::NORMAL, classify_zone(DMA_LIMIT), "DMA_LIMIT is NORMAL");
    TEST_ASSERT_EQ(ZoneType::NORMAL, classify_zone(HIGH_LIMIT - 1), "just below HIGH_LIMIT is NORMAL");
    return nullptr;
}

static const char* test_classify_high() {
    TEST_ASSERT_EQ(ZoneType::HIGH, classify_zone(HIGH_LIMIT), "HIGH_LIMIT is HIGH");
    TEST_ASSERT_EQ(ZoneType::HIGH, classify_zone(HIGH_LIMIT + FRAME_SIZE), "above HIGH_LIMIT is HIGH");
    return nullptr;
}

static const char* test_zone_limit_dma() {
    TEST_ASSERT_EQ(DMA_LIMIT, zone_limit(ZoneType::DMA), "DMA limit");
    return nullptr;
}

static const char* test_zone_limit_normal() {
    TEST_ASSERT_EQ(HIGH_LIMIT, zone_limit(ZoneType::NORMAL), "NORMAL limit");
    return nullptr;
}

static const char* test_zone_limit_high() {
    TEST_ASSERT_EQ((uintptr_t)UINTPTR_MAX, zone_limit(ZoneType::HIGH), "HIGH limit is UINTPTR_MAX");
    return nullptr;
}

/* ---- Zone::populate_zone / accessors ---- */

static const char* test_zone_default_uninitialized() {
    Zone z;
    // Accessing uninitialized zone returns 0 or NORMAL gracefully.
    TEST_ASSERT_EQ(0u, z.base(),        "default base is 0");
    TEST_ASSERT_EQ(0u, z.length(),      "default length is 0");
    TEST_ASSERT_EQ(0u, z.frame_count(), "default frame_count is 0");
    return nullptr;
}

static const char* test_zone_populate_normal() {
    Zone z;
    uintptr_t base   = DMA_LIMIT;            // 16 MB
    size_t    length = 64 * FRAME_SIZE;      // 64 pages
    z.populate_zone(base, length, ZoneType::NORMAL);

    TEST_ASSERT_EQ(base,            z.base(),        "base after populate");
    TEST_ASSERT_EQ(length,          z.length(),      "length after populate");
    TEST_ASSERT_EQ(64u,             z.frame_count(), "frame_count = 64");
    TEST_ASSERT_EQ(ZoneType::NORMAL, z.type(),       "type is NORMAL");
    return nullptr;
}

static const char* test_zone_populate_dma() {
    Zone z;
    uintptr_t base   = 0;
    size_t    length = 16 * FRAME_SIZE;
    z.populate_zone(base, length, ZoneType::DMA);

    TEST_ASSERT_EQ(base,          z.base(),        "DMA base");
    TEST_ASSERT_EQ(length,        z.length(),      "DMA length");
    TEST_ASSERT_EQ(16u,           z.frame_count(), "DMA frame_count");
    TEST_ASSERT_EQ(ZoneType::DMA, z.type(),        "type is DMA");
    return nullptr;
}

static const char* test_zone_populate_high() {
    Zone z;
    uintptr_t base   = HIGH_LIMIT;          // 4 GB
    size_t    length = 128 * FRAME_SIZE;
    z.populate_zone(base, length, ZoneType::HIGH);

    TEST_ASSERT_EQ(base,           z.base(),        "HIGH base");
    TEST_ASSERT_EQ(length,         z.length(),      "HIGH length");
    TEST_ASSERT_EQ(128u,           z.frame_count(), "HIGH frame_count");
    TEST_ASSERT_EQ(ZoneType::HIGH, z.type(),        "type is HIGH");
    return nullptr;
}

static const char* test_zone_ctor_fields() {
    // Constructor version sets all fields immediately.
    uintptr_t base   = DMA_LIMIT + FRAME_SIZE;
    size_t    length = 32 * FRAME_SIZE;
    Zone z(base, length, ZoneType::NORMAL);

    TEST_ASSERT_EQ(base,            z.base(),        "ctor base");
    TEST_ASSERT_EQ(length,          z.length(),      "ctor length");
    TEST_ASSERT_EQ(32u,             z.frame_count(), "ctor frame_count");
    TEST_ASSERT_EQ(ZoneType::NORMAL, z.type(),       "ctor type");
    return nullptr;
}

static const char* test_zone_frame_count_math() {
    Zone z;
    size_t pages = 256;
    z.populate_zone(0, pages * FRAME_SIZE, ZoneType::DMA);
    TEST_ASSERT_EQ(pages, z.frame_count(), "frame_count = length / FRAME_SIZE");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"classify_dma",                 test_classify_dma},
    {"classify_normal",              test_classify_normal},
    {"classify_high",                test_classify_high},
    {"zone_limit_dma",               test_zone_limit_dma},
    {"zone_limit_normal",            test_zone_limit_normal},
    {"zone_limit_high",              test_zone_limit_high},
    {"zone_default_uninitialized",   test_zone_default_uninitialized},
    {"zone_populate_normal",         test_zone_populate_normal},
    {"zone_populate_dma",            test_zone_populate_dma},
    {"zone_populate_high",           test_zone_populate_high},
    {"zone_ctor_fields",             test_zone_ctor_fields},
    {"zone_frame_count_math",        test_zone_frame_count_math},
};

int run_kernel_zone_allocator_tests() {
    return run_tests("Kernel::ZoneAllocator",
                     s_tests,
                     sizeof(s_tests) / sizeof(s_tests[0]));
}
