#include <tests/test_framework.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Container/Associative/set.h>
#include <LibFK/Container/Associative/map.h>
#include <LibFK/Container/Adapters/priority_queue.h>
#include <LibFK/Container/Sequence/deque.h>
#include <LibFK/Container/Associative/hash_map.h>

using namespace fk::containers;

/* ---- Vector ---- */

static const char* test_vector_push_pop() {
    Vector<int> v;
    TEST_ASSERT(v.is_empty(), "Vector should be empty initially");
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    TEST_ASSERT_EQ(3, (int)v.size(), "Size should be 3");
    TEST_ASSERT_EQ(3, v[2], "Third element should be 3");
    v.pop_back();
    TEST_ASSERT_EQ(2, (int)v.size(), "Size should be 2 after pop");
    return NULL;
}

static const char* test_vector_insert_remove_at() {
    Vector<int> v;
    v.push_back(1);
    v.push_back(3);
    v.insert_at(1, 2);
    TEST_ASSERT_EQ(3, (int)v.size(), "Size should be 3 after insert_at");
    TEST_ASSERT_EQ(2, v[1], "Inserted element should be 2");
    v.remove_at(0);
    TEST_ASSERT_EQ(2, v[0], "After remove_at(0), first elem should be 2");
    return NULL;
}

static const char* test_vector_grow() {
    Vector<int> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);
    TEST_ASSERT_EQ(100, (int)v.size(), "Vector should hold 100 elements");
    TEST_ASSERT_EQ(50, v[50], "Element at 50 should be 50");
    return NULL;
}

/* ---- Set ---- */

static const char* test_set_insert_contains() {
    Set<int> s;
    TEST_ASSERT(s.is_empty(), "Set should be empty");
    TEST_ASSERT(s.insert(5), "Insert 5 should succeed");
    TEST_ASSERT(s.insert(3), "Insert 3 should succeed");
    TEST_ASSERT(s.insert(7), "Insert 7 should succeed");
    TEST_ASSERT(!s.insert(3), "Duplicate insert should return false");
    TEST_ASSERT_EQ(3, (int)s.size(), "Set size should be 3");
    TEST_ASSERT(s.contains(5), "Set should contain 5");
    TEST_ASSERT(s.contains(3), "Set should contain 3");
    TEST_ASSERT(!s.contains(4), "Set should not contain 4");
    return NULL;
}

static const char* test_set_remove() {
    Set<int> s;
    s.insert(1); s.insert(2); s.insert(3);
    TEST_ASSERT(s.remove(2), "Remove 2 should succeed");
    TEST_ASSERT(!s.contains(2), "2 should be gone");
    TEST_ASSERT(!s.remove(99), "Remove non-existent should return false");
    TEST_ASSERT_EQ(2, (int)s.size(), "Size should be 2");
    return NULL;
}

static const char* test_set_sorted_order() {
    Set<int> s;
    s.insert(5); s.insert(1); s.insert(3); s.insert(2); s.insert(4);
    int prev = -1;
    for (auto x : s) {
        TEST_ASSERT(x > prev, "Set should iterate in sorted order");
        prev = x;
    }
    return NULL;
}

/* ---- Map ---- */

static const char* test_map_insert_get() {
    Map<int, int> m;
    TEST_ASSERT(m.is_empty(), "Map should be empty");
    m.insert(1, 10);
    m.insert(2, 20);
    m.insert(3, 30);
    TEST_ASSERT_EQ(3, (int)m.size(), "Map size should be 3");
    auto v = m.get(2);
    TEST_ASSERT(v.has_value(), "get(2) should return value");
    TEST_ASSERT_EQ(20, v.value(), "get(2) should return 20");
    TEST_ASSERT(!m.get(99).has_value(), "get(99) should be empty");
    return NULL;
}

static const char* test_map_operator_bracket() {
    Map<int, int> m;
    m[5] = 50;
    m[3] = 30;
    TEST_ASSERT_EQ(50, m[5], "m[5] should be 50");
    TEST_ASSERT_EQ(30, m[3], "m[3] should be 30");
    m[5] = 55;
    TEST_ASSERT_EQ(55, m[5], "m[5] should be 55 after update");
    return NULL;
}

static const char* test_map_remove() {
    Map<int, int> m;
    m.insert(1, 10); m.insert(2, 20);
    TEST_ASSERT(m.remove(1), "Remove key 1 should succeed");
    TEST_ASSERT(!m.contains(1), "Key 1 should be gone");
    TEST_ASSERT_EQ(1, (int)m.size(), "Size should be 1");
    return NULL;
}

/* ---- PriorityQueue ---- */

static const char* test_priority_queue_max_heap() {
    PriorityQueue<int> pq;
    TEST_ASSERT(pq.is_empty(), "PQ should be empty");
    pq.push(3); pq.push(1); pq.push(4); pq.push(1); pq.push(5); pq.push(9);
    TEST_ASSERT_EQ(6, (int)pq.size(), "PQ size should be 6");
    TEST_ASSERT_EQ(9, pq.top(), "Top should be 9 (max)");
    pq.pop();
    TEST_ASSERT_EQ(5, pq.top(), "Top should be 5 after pop");
    pq.pop();
    TEST_ASSERT_EQ(4, pq.top(), "Top should be 4 after pop");
    return NULL;
}

static const char* test_priority_queue_sorted_drain() {
    PriorityQueue<int> pq;
    pq.push(5); pq.push(2); pq.push(8); pq.push(1); pq.push(9);
    int prev = 999;
    while (!pq.is_empty()) {
        TEST_ASSERT(pq.top() <= prev, "Elements should come out in descending order");
        prev = pq.top();
        pq.pop();
    }
    return NULL;
}

/* ---- Deque ---- */

static const char* test_deque_push_pop() {
    Deque<int> d;
    TEST_ASSERT(d.is_empty(), "Deque should be empty");
    d.push_back(1);
    d.push_back(2);
    d.push_front(0);
    TEST_ASSERT_EQ(3, (int)d.size(), "Size should be 3");
    TEST_ASSERT_EQ(0, d.front(), "Front should be 0");
    TEST_ASSERT_EQ(2, d.back(), "Back should be 2");
    d.pop_front();
    TEST_ASSERT_EQ(1, d.front(), "Front should be 1 after pop_front");
    d.pop_back();
    TEST_ASSERT_EQ(1, d.back(), "Back should be 1 after pop_back");
    return NULL;
}

/* ---- HashMap ---- */

static const char* test_hashmap_basic() {
    HashMap<int, int> m;
    TEST_ASSERT(m.is_empty(), "HashMap should be empty");
    m.insert(1, 100);
    m.insert(2, 200);
    TEST_ASSERT_EQ(2, (int)m.size(), "HashMap size should be 2");
    auto v = m.get(1);
    TEST_ASSERT(v.has_value(), "get(1) should succeed");
    TEST_ASSERT_EQ(100, v.value(), "get(1) should return 100");
    return NULL;
}

static const char* test_hashmap_remove_tombstone() {
    HashMap<int, int> m;
    for (int i = 0; i < 10; ++i) m.insert(i, i * 10);
    m.remove(5);
    TEST_ASSERT(!m.contains(5), "5 should be removed");
    TEST_ASSERT(m.contains(4), "4 should still exist");
    TEST_ASSERT(m.contains(6), "6 should still exist (probe chain intact)");
    return NULL;
}

static test_case_t s_container_tests[] = {
    {"test_vector_push_pop",           test_vector_push_pop},
    {"test_vector_insert_remove_at",   test_vector_insert_remove_at},
    {"test_vector_grow",               test_vector_grow},
    {"test_set_insert_contains",       test_set_insert_contains},
    {"test_set_remove",                test_set_remove},
    {"test_set_sorted_order",          test_set_sorted_order},
    {"test_map_insert_get",            test_map_insert_get},
    {"test_map_operator_bracket",      test_map_operator_bracket},
    {"test_map_remove",                test_map_remove},
    {"test_priority_queue_max_heap",   test_priority_queue_max_heap},
    {"test_priority_queue_sorted_drain", test_priority_queue_sorted_drain},
    {"test_deque_push_pop",            test_deque_push_pop},
    {"test_hashmap_basic",             test_hashmap_basic},
    {"test_hashmap_remove_tombstone",  test_hashmap_remove_tombstone},
};

int run_libfk_container_advanced_tests() {
    return run_tests("LibFK Containers (Advanced)",
                     s_container_tests,
                     sizeof(s_container_tests) / sizeof(s_container_tests[0]));
}
