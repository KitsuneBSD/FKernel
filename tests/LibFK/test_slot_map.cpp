#include <tests/test_framework.h>
#include <LibFK/Container/slot_map.h>

using Map = fk::containers::SlotMap<int>;
using Key = Map::Key;

static const char* test_insert_get() {
    Map m;
    auto k1 = m.insert(10);
    auto k2 = m.insert(20);
    TEST_ASSERT(!k1.is_error(), "insert 10 ok");
    TEST_ASSERT(!k2.is_error(), "insert 20 ok");
    TEST_ASSERT_EQ(2u, m.size(), "size is 2");

    int* v1 = m.get(k1.value());
    int* v2 = m.get(k2.value());
    TEST_ASSERT(v1 != nullptr, "get k1 non-null");
    TEST_ASSERT(v2 != nullptr, "get k2 non-null");
    TEST_ASSERT_EQ(10, *v1, "k1 value is 10");
    TEST_ASSERT_EQ(20, *v2, "k2 value is 20");
    return nullptr;
}

static const char* test_remove_stale() {
    Map m;
    auto k = m.insert(42).value();
    TEST_ASSERT(m.contains(k), "key valid before remove");

    bool removed = m.remove(k);
    TEST_ASSERT(removed, "remove returns true");
    TEST_ASSERT_EQ(0u, m.size(), "size is 0");
    TEST_ASSERT(!m.contains(k), "key stale after remove");
    TEST_ASSERT(m.get(k) == nullptr, "get returns null for stale key");

    bool removed_again = m.remove(k);
    TEST_ASSERT(!removed_again, "remove stale key returns false");
    return nullptr;
}

static const char* test_slot_reuse_generation() {
    Map m;
    auto k1 = m.insert(1).value();
    m.remove(k1);
    auto k2 = m.insert(2).value();

    // k2 should reuse the same slot but different generation
    TEST_ASSERT_EQ(k1.index, k2.index, "slot index reused");
    TEST_ASSERT(k1.generation != k2.generation, "generation differs");

    // k1 is stale — must not find value 2
    TEST_ASSERT(m.get(k1) == nullptr, "stale key k1 returns null");
    TEST_ASSERT_EQ(2, *m.get(k2), "k2 returns new value");
    return nullptr;
}

static const char* test_invalid_key() {
    Map m;
    Key inv = Key::invalid();
    TEST_ASSERT(!inv.is_valid(), "invalid key is_valid=false");
    TEST_ASSERT(m.get(inv) == nullptr, "get invalid returns null");
    TEST_ASSERT(!m.remove(inv), "remove invalid returns false");
    return nullptr;
}

static const char* test_for_each() {
    Map m;
    auto k1 = m.insert(10).value();
    auto k2 = m.insert(20).value();
    auto k3 = m.insert(30).value();
    m.remove(k2); // leave a hole

    int sum = 0;
    int count = 0;
    m.for_each([&](Key, int& v) {
        sum += v;
        ++count;
    });
    TEST_ASSERT_EQ(2, count, "for_each visits 2 active slots");
    TEST_ASSERT_EQ(40, sum, "sum is 10+30=40");
    (void)k1; (void)k3;
    return nullptr;
}

static const char* test_clear() {
    Map m;
    m.insert(1);
    m.insert(2);
    m.insert(3);
    TEST_ASSERT_EQ(3u, m.size(), "3 items before clear");
    m.clear();
    TEST_ASSERT_EQ(0u, m.size(), "0 items after clear");
    TEST_ASSERT(m.is_empty(), "is_empty after clear");
    return nullptr;
}

static const char* test_out_of_bounds_key() {
    Map m;
    m.insert(1);
    Key oob{9999u, 1u};
    TEST_ASSERT(m.get(oob) == nullptr, "out-of-bounds index returns null");
    TEST_ASSERT(!m.remove(oob), "remove out-of-bounds returns false");
    return nullptr;
}

static const char* test_many_insertions() {
    Map m;
    Key keys[50];
    for (int i = 0; i < 50; ++i)
        keys[i] = m.insert(i * 2).value();
    TEST_ASSERT_EQ(50u, m.size(), "50 items inserted");
    for (int i = 0; i < 50; i += 2)
        m.remove(keys[i]);
    TEST_ASSERT_EQ(25u, m.size(), "25 items after removing even indices");
    for (int i = 1; i < 50; i += 2)
        TEST_ASSERT_EQ(i * 2, *m.get(keys[i]), "odd-index values intact");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"insert_get",               test_insert_get},
    {"remove_stale",             test_remove_stale},
    {"slot_reuse_generation",    test_slot_reuse_generation},
    {"invalid_key",              test_invalid_key},
    {"for_each",                 test_for_each},
    {"clear",                    test_clear},
    {"out_of_bounds_key",        test_out_of_bounds_key},
    {"many_insertions",          test_many_insertions},
};

int run_libfk_slot_map_tests() {
    return run_tests("LibFK::SlotMap", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
