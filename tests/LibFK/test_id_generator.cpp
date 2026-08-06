#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/id_generator.h>

// Minimal strong-typedef ID for testing
struct TestId {
    explicit TestId(uint64_t v = 0) : m_v(v) {}
    uint64_t value() const { return m_v; }
    bool operator==(TestId o) const { return m_v == o.m_v; }
    bool operator!=(TestId o) const { return m_v != o.m_v; }
private:
    uint64_t m_v;
};

using Gen = fk::algorithms::IdGenerator<TestId>;

static const char* test_sequential_ids() {
    Gen g;
    TestId a = g.generate();
    TestId b = g.generate();
    TestId c = g.generate();
    TEST_ASSERT_EQ(1u, a.value(), "first id is 1");
    TEST_ASSERT_EQ(2u, b.value(), "second id is 2");
    TEST_ASSERT_EQ(3u, c.value(), "third id is 3");
    return nullptr;
}

static const char* test_custom_start() {
    Gen g(100);
    TEST_ASSERT_EQ(100u, g.generate().value(), "custom start at 100");
    TEST_ASSERT_EQ(101u, g.generate().value(), "next is 101");
    return nullptr;
}

static const char* test_is_allocated() {
    Gen g;
    g.generate(); // id 1
    g.generate(); // id 2
    TEST_ASSERT(g.is_allocated(TestId(1)), "id 1 is allocated");
    TEST_ASSERT(g.is_allocated(TestId(2)), "id 2 is allocated");
    TEST_ASSERT(!g.is_allocated(TestId(3)), "id 3 not yet allocated");
    TEST_ASSERT(!g.is_allocated(TestId(0)), "id 0 never allocated");
    return nullptr;
}

static const char* test_reset() {
    Gen g;
    g.generate(); g.generate();
    g.reset();
    TEST_ASSERT_EQ(1u, g.generate().value(), "reset restarts at 1");
    return nullptr;
}

static const char* test_max_limit() {
    Gen g(1, 3); // max=3 so valid ids are 1,2; id 3 wraps
    TestId a = g.generate();
    TestId b = g.generate();
    TestId overflow = g.generate();
    TEST_ASSERT_EQ(1u, a.value(), "id 1 ok");
    TEST_ASSERT_EQ(2u, b.value(), "id 2 ok");
    TEST_ASSERT_EQ(0u, overflow.value(), "overflow returns 0 sentinel");
    return nullptr;
}

static const char* test_next_value() {
    Gen g;
    TEST_ASSERT_EQ(1u, g.next_value(), "next_value starts at 1");
    g.generate();
    TEST_ASSERT_EQ(2u, g.next_value(), "next_value is 2 after one generate");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"sequential ids",  test_sequential_ids},
    {"custom start",    test_custom_start},
    {"is_allocated",    test_is_allocated},
    {"reset",           test_reset},
    {"max limit",       test_max_limit},
    {"next_value",      test_next_value},
};

int run_libfk_id_generator_tests() {
    return run_tests("LibFK::IdGenerator", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
