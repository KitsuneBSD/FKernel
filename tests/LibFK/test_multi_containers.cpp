#include <tests/test_framework.h>
#include <LibFK/Container/multi_set.h>
#include <LibFK/Container/multi_map.h>

using namespace fk::containers;

static const char* test_multiset_insert_contains() {
    MultiSet<int> ms;
    ms.insert(1);
    ms.insert(2);
    ms.insert(1);
    TEST_ASSERT(ms.contains(1), "should contain 1");
    TEST_ASSERT(ms.contains(2), "should contain 2");
    TEST_ASSERT(!ms.contains(3), "should not contain 3");
    return NULL;
}

static const char* test_multiset_count() {
    MultiSet<int> ms;
    ms.insert(5);
    ms.insert(5);
    ms.insert(5);
    ms.insert(7);
    TEST_ASSERT_EQ(3, (int)ms.count(5), "count(5) should be 3");
    TEST_ASSERT_EQ(1, (int)ms.count(7), "count(7) should be 1");
    TEST_ASSERT_EQ(0, (int)ms.count(9), "count(9) should be 0");
    return NULL;
}

static const char* test_multiset_remove_one() {
    MultiSet<int> ms;
    ms.insert(3);
    ms.insert(3);
    TEST_ASSERT_EQ(2, (int)ms.count(3), "before remove: count 2");
    ms.remove(3);
    TEST_ASSERT_EQ(1, (int)ms.count(3), "after remove: count 1");
    return NULL;
}

static const char* test_multiset_remove_all() {
    MultiSet<int> ms;
    ms.insert(4);
    ms.insert(4);
    ms.insert(4);
    size_t removed = ms.remove_all(4);
    TEST_ASSERT_EQ(3, (int)removed, "should remove 3 elements");
    TEST_ASSERT(!ms.contains(4), "no 4 should remain");
    return NULL;
}

static const char* test_multiset_sorted_order() {
    MultiSet<int> ms;
    ms.insert(3); ms.insert(1); ms.insert(2); ms.insert(1);
    int prev = -1;
    for (const int& v : ms) {
        TEST_ASSERT(v >= prev, "elements must be non-decreasing");
        prev = v;
    }
    return NULL;
}

static const char* test_multimap_insert_contains() {
    MultiMap<int, int> mm;
    mm.insert(1, 10);
    mm.insert(1, 20);
    mm.insert(2, 30);
    TEST_ASSERT(mm.contains(1), "should contain key 1");
    TEST_ASSERT(mm.contains(2), "should contain key 2");
    TEST_ASSERT(!mm.contains(3), "should not contain key 3");
    return NULL;
}

static const char* test_multimap_count() {
    MultiMap<int, int> mm;
    mm.insert(5, 1);
    mm.insert(5, 2);
    mm.insert(5, 3);
    mm.insert(6, 9);
    TEST_ASSERT_EQ(3, (int)mm.count(5), "count(5) should be 3");
    TEST_ASSERT_EQ(1, (int)mm.count(6), "count(6) should be 1");
    return NULL;
}

static const char* test_multimap_get_all() {
    MultiMap<int, int> mm;
    mm.insert(7, 100);
    mm.insert(7, 200);
    mm.insert(7, 300);
    auto vals = mm.get_all(7);
    TEST_ASSERT_EQ(3, (int)vals.size(), "get_all should return 3 values");
    TEST_ASSERT_EQ(100, vals[0], "first value should be 100");
    TEST_ASSERT_EQ(200, vals[1], "second value should be 200");
    TEST_ASSERT_EQ(300, vals[2], "third value should be 300");
    return NULL;
}

static const char* test_multimap_remove_one() {
    MultiMap<int, int> mm;
    mm.insert(8, 1);
    mm.insert(8, 2);
    TEST_ASSERT_EQ(2, (int)mm.count(8), "before remove: count 2");
    mm.remove(8);
    TEST_ASSERT_EQ(1, (int)mm.count(8), "after remove: count 1");
    return NULL;
}

static const char* test_multimap_remove_all() {
    MultiMap<int, int> mm;
    mm.insert(9, 1); mm.insert(9, 2); mm.insert(9, 3);
    size_t removed = mm.remove_all(9);
    TEST_ASSERT_EQ(3, (int)removed, "remove_all should return 3");
    TEST_ASSERT(!mm.contains(9), "no key 9 should remain");
    return NULL;
}

static const test_case_t s_multi_container_tests[] = {
    {"test_multiset_insert_contains", test_multiset_insert_contains},
    {"test_multiset_count",           test_multiset_count},
    {"test_multiset_remove_one",      test_multiset_remove_one},
    {"test_multiset_remove_all",      test_multiset_remove_all},
    {"test_multiset_sorted_order",    test_multiset_sorted_order},
    {"test_multimap_insert_contains", test_multimap_insert_contains},
    {"test_multimap_count",           test_multimap_count},
    {"test_multimap_get_all",         test_multimap_get_all},
    {"test_multimap_remove_one",      test_multimap_remove_one},
    {"test_multimap_remove_all",      test_multimap_remove_all},
};

int run_libfk_multi_container_tests() {
    return run_tests("LibFK Multi-Containers",
                     s_multi_container_tests,
                     sizeof(s_multi_container_tests) / sizeof(s_multi_container_tests[0]));
}
