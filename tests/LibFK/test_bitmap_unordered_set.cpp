#include <tests/test_framework.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Container/unordered_set.h>

using namespace fk::containers;

/* ---- Bitmap ---- */

static const char* test_bitmap_initial_all_clear() {
    uint64_t storage[2] = {};
    Bitmap<uint64_t> bm(storage, 128);
    for (size_t i = 0; i < 128; ++i)
        TEST_ASSERT(!bm.get(i), "All bits should be clear after construction");
    return NULL;
}

static const char* test_bitmap_set_get() {
    uint64_t storage[2] = {};
    Bitmap<uint64_t> bm(storage, 128);
    bm.set(0, true);
    bm.set(63, true);
    bm.set(64, true);
    bm.set(127, true);
    TEST_ASSERT(bm.get(0),   "Bit 0 should be set");
    TEST_ASSERT(bm.get(63),  "Bit 63 should be set");
    TEST_ASSERT(bm.get(64),  "Bit 64 should be set");
    TEST_ASSERT(bm.get(127), "Bit 127 should be set");
    TEST_ASSERT(!bm.get(1),  "Bit 1 should be clear");
    TEST_ASSERT(!bm.get(65), "Bit 65 should be clear");
    return NULL;
}

static const char* test_bitmap_clear_single() {
    uint64_t storage[1] = {};
    Bitmap<uint64_t> bm(storage, 64);
    bm.set(5, true);
    bm.set(10, true);
    TEST_ASSERT(bm.get(5),  "Bit 5 should be set");
    bm.clear(5);
    TEST_ASSERT(!bm.get(5), "Bit 5 should be cleared");
    TEST_ASSERT(bm.get(10), "Bit 10 should still be set");
    return NULL;
}

static const char* test_bitmap_clear_all() {
    uint64_t storage[2] = {};
    Bitmap<uint64_t> bm(storage, 128);
    bm.set(0, true);
    bm.set(64, true);
    bm.set(127, true);
    bm.clear_all();
    for (size_t i = 0; i < 128; ++i)
        TEST_ASSERT(!bm.get(i), "All bits should be clear after clear_all");
    return NULL;
}

static const char* test_bitmap_alloc_sequential() {
    uint64_t storage[1] = {};
    Bitmap<uint64_t> bm(storage, 64);
    ssize_t a = bm.alloc();
    TEST_ASSERT_EQ(0, (int)a, "First alloc should return bit 0");
    ssize_t b = bm.alloc();
    TEST_ASSERT_EQ(1, (int)b, "Second alloc should return bit 1");
    ssize_t c = bm.alloc();
    TEST_ASSERT_EQ(2, (int)c, "Third alloc should return bit 2");
    TEST_ASSERT(bm.get(0) && bm.get(1) && bm.get(2), "Allocated bits should be set");
    return NULL;
}

static const char* test_bitmap_alloc_skips_used() {
    uint64_t storage[1] = {};
    Bitmap<uint64_t> bm(storage, 64);
    bm.set(0, true);
    bm.set(1, true);
    ssize_t a = bm.alloc();
    TEST_ASSERT_EQ(2, (int)a, "Alloc should skip bits 0 and 1");
    return NULL;
}

static const char* test_bitmap_alloc_full_returns_minus_one() {
    uint32_t storage[1] = {};
    Bitmap<uint32_t> bm(storage, 32);
    for (int i = 0; i < 32; ++i)
        bm.alloc();
    ssize_t r = bm.alloc();
    TEST_ASSERT_EQ(-1, (int)r, "alloc on full bitmap should return -1");
    return NULL;
}

static const char* test_bitmap_set_false_clears() {
    uint64_t storage[1] = {};
    Bitmap<uint64_t> bm(storage, 64);
    bm.set(7, true);
    TEST_ASSERT(bm.get(7), "Bit 7 should be set");
    bm.set(7, false);
    TEST_ASSERT(!bm.get(7), "Bit 7 should be cleared via set(false)");
    return NULL;
}

/* ---- UnorderedSet ---- */

static const char* test_unorderedset_insert_contains() {
    UnorderedSet<int> s;
    TEST_ASSERT(s.is_empty(), "Set should be empty initially");
    TEST_ASSERT(s.insert(10), "Insert 10 should succeed");
    TEST_ASSERT(s.insert(20), "Insert 20 should succeed");
    TEST_ASSERT(s.insert(30), "Insert 30 should succeed");
    TEST_ASSERT_EQ(3u, s.size(), "Size should be 3");
    TEST_ASSERT(s.contains(10), "Should contain 10");
    TEST_ASSERT(s.contains(20), "Should contain 20");
    TEST_ASSERT(s.contains(30), "Should contain 30");
    TEST_ASSERT(!s.contains(99), "Should not contain 99");
    return NULL;
}

static const char* test_unorderedset_duplicate_insert() {
    UnorderedSet<int> s;
    TEST_ASSERT(s.insert(5), "First insert should succeed");
    TEST_ASSERT(!s.insert(5), "Duplicate insert should return false");
    TEST_ASSERT_EQ(1u, s.size(), "Size should still be 1");
    return NULL;
}

static const char* test_unorderedset_remove() {
    UnorderedSet<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    TEST_ASSERT(s.remove(2), "Remove 2 should succeed");
    TEST_ASSERT_EQ(2u, s.size(), "Size should be 2 after remove");
    TEST_ASSERT(!s.contains(2), "Should not contain 2 after remove");
    TEST_ASSERT(s.contains(1), "Should still contain 1");
    TEST_ASSERT(s.contains(3), "Should still contain 3");
    return NULL;
}

static const char* test_unorderedset_remove_nonexistent() {
    UnorderedSet<int> s;
    s.insert(10);
    TEST_ASSERT(!s.remove(99), "Remove of non-existent should return false");
    TEST_ASSERT_EQ(1u, s.size(), "Size should be unchanged");
    return NULL;
}

static const char* test_unorderedset_empty_after_removes() {
    UnorderedSet<int> s;
    s.insert(1);
    s.insert(2);
    s.remove(1);
    s.remove(2);
    TEST_ASSERT(s.is_empty(), "Set should be empty after removing all elements");
    TEST_ASSERT_EQ(0u, s.size(), "Size should be 0");
    return NULL;
}

static const char* test_unorderedset_many_elements() {
    UnorderedSet<int> s;
    for (int i = 0; i < 50; ++i)
        s.insert(i);
    TEST_ASSERT_EQ(50u, s.size(), "Size should be 50");
    for (int i = 0; i < 50; ++i)
        TEST_ASSERT(s.contains(i), "Should contain all inserted elements");
    TEST_ASSERT(!s.contains(50), "Should not contain 50");
    return NULL;
}

static const char* test_unorderedset_reinsert_after_remove() {
    UnorderedSet<int> s;
    s.insert(42);
    s.remove(42);
    TEST_ASSERT(!s.contains(42), "Should not contain 42 after remove");
    TEST_ASSERT(s.insert(42), "Re-insert after remove should succeed");
    TEST_ASSERT(s.contains(42), "Should contain 42 after re-insert");
    return NULL;
}

/* ---- Registration ---- */

static test_case_t s_tests[] = {
    // Bitmap
    {"test_bitmap_initial_all_clear",          test_bitmap_initial_all_clear},
    {"test_bitmap_set_get",                    test_bitmap_set_get},
    {"test_bitmap_clear_single",               test_bitmap_clear_single},
    {"test_bitmap_clear_all",                  test_bitmap_clear_all},
    {"test_bitmap_alloc_sequential",           test_bitmap_alloc_sequential},
    {"test_bitmap_alloc_skips_used",           test_bitmap_alloc_skips_used},
    {"test_bitmap_alloc_full_returns_minus_one", test_bitmap_alloc_full_returns_minus_one},
    {"test_bitmap_set_false_clears",           test_bitmap_set_false_clears},
    // UnorderedSet
    {"test_unorderedset_insert_contains",      test_unorderedset_insert_contains},
    {"test_unorderedset_duplicate_insert",     test_unorderedset_duplicate_insert},
    {"test_unorderedset_remove",               test_unorderedset_remove},
    {"test_unorderedset_remove_nonexistent",   test_unorderedset_remove_nonexistent},
    {"test_unorderedset_empty_after_removes",  test_unorderedset_empty_after_removes},
    {"test_unorderedset_many_elements",        test_unorderedset_many_elements},
    {"test_unorderedset_reinsert_after_remove", test_unorderedset_reinsert_after_remove},
};

int run_libfk_bitmap_unordered_set_tests() {
    return run_tests("LibFK Bitmap/UnorderedSet",
                     s_tests,
                     sizeof(s_tests) / sizeof(s_tests[0]));
}
