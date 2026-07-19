#include <tests/test_framework.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/optional.h>

struct Counted : public fk::memory::RefCounted<Counted> {
    int value;
    static int alive;
    explicit Counted(int v) : value(v) { ++alive; }
    ~Counted() { --alive; }
};
int Counted::alive = 0;

static const char* test_own_ptr_basic() {
    Counted::alive = 0;
    {
        auto p = fk::make_owned<Counted>(42);
        TEST_ASSERT(static_cast<bool>(p), "OwnPtr should be non-null");
        TEST_ASSERT_EQ(42, p->value, "OwnPtr value should be 42");
        TEST_ASSERT_EQ(1, Counted::alive, "Should have 1 alive");
    }
    TEST_ASSERT_EQ(0, Counted::alive, "Should have 0 alive after scope");
    return NULL;
}

static const char* test_own_ptr_move() {
    Counted::alive = 0;
    auto a = fk::make_owned<Counted>(7);
    auto b = fk::types::move(a);
    TEST_ASSERT(!static_cast<bool>(a), "a should be null after move");
    TEST_ASSERT(static_cast<bool>(b), "b should be non-null after move");
    TEST_ASSERT_EQ(7, b->value, "b value should be 7");
    TEST_ASSERT_EQ(1, Counted::alive, "1 object alive after move");
    return NULL;
}

static const char* test_own_ptr_clear() {
    Counted::alive = 0;
    auto p = fk::make_owned<Counted>(1);
    p.clear();
    TEST_ASSERT(!static_cast<bool>(p), "OwnPtr should be null after clear");
    TEST_ASSERT_EQ(0, Counted::alive, "0 alive after clear");
    return NULL;
}

static const char* test_own_ptr_leak() {
    Counted::alive = 0;
    auto p = fk::make_owned<Counted>(99);
    Counted* raw = p.leak_ptr();
    TEST_ASSERT(!static_cast<bool>(p), "OwnPtr should be null after leak");
    TEST_ASSERT_EQ(99, raw->value, "Raw pointer value should be 99");
    TEST_ASSERT_EQ(1, Counted::alive, "1 alive after leak");
    delete raw;
    TEST_ASSERT_EQ(0, Counted::alive, "0 alive after manual delete");
    return NULL;
}

static const char* test_ref_ptr_adopt_basic() {
    Counted::alive = 0;
    {
        auto a = fk::adopt_ref(new Counted(5));
        TEST_ASSERT_EQ(1, (int)a->ref_count(), "adopt_ref starts at ref_count 1");
        TEST_ASSERT_EQ(1, Counted::alive, "1 object alive");
    }
    TEST_ASSERT_EQ(0, Counted::alive, "0 alive after RefPtr destroyed");
    return NULL;
}

static const char* test_ref_ptr_copy() {
    Counted::alive = 0;
    auto a = fk::adopt_ref(new Counted(10));
    {
        fk::RefPtr<Counted> b = a;
        TEST_ASSERT_EQ(2, (int)a->ref_count(), "ref_count should be 2 with two RefPtrs");
        TEST_ASSERT_EQ(1, Counted::alive, "still 1 object alive");
    }
    TEST_ASSERT_EQ(1, (int)a->ref_count(), "ref_count back to 1 after b destroyed");
    TEST_ASSERT_EQ(1, Counted::alive, "still 1 alive with a");
    return NULL;
}

static const char* test_ref_ptr_move() {
    Counted::alive = 0;
    auto a = fk::adopt_ref(new Counted(20));
    auto b = fk::types::move(a);
    TEST_ASSERT(!static_cast<bool>(a), "a null after move");
    TEST_ASSERT(static_cast<bool>(b), "b non-null after move");
    TEST_ASSERT_EQ(20, b->value, "moved value should be 20");
    TEST_ASSERT_EQ(1, Counted::alive, "1 alive after move");
    return NULL;
}

static const char* test_optional_empty() {
    fk::memory::optional<int> empty;
    TEST_ASSERT(!empty.has_value(), "optional should have no value");
    return NULL;
}

static const char* test_optional_filled() {
    fk::memory::optional<int> opt(42);
    TEST_ASSERT(opt.has_value(), "optional should have value");
    TEST_ASSERT_EQ(42, opt.value(), "optional value should be 42");
    return NULL;
}

static const char* test_optional_reset() {
    fk::memory::optional<int> opt(10);
    opt.reset();
    TEST_ASSERT(!opt.has_value(), "optional should have no value after reset");
    return NULL;
}

static const test_case_t smart_ptr_tests[] = {
    {"test_own_ptr_basic",       test_own_ptr_basic},
    {"test_own_ptr_move",        test_own_ptr_move},
    {"test_own_ptr_clear",       test_own_ptr_clear},
    {"test_own_ptr_leak",        test_own_ptr_leak},
    {"test_ref_ptr_adopt_basic", test_ref_ptr_adopt_basic},
    {"test_ref_ptr_copy",        test_ref_ptr_copy},
    {"test_ref_ptr_move",        test_ref_ptr_move},
    {"test_optional_empty",      test_optional_empty},
    {"test_optional_filled",     test_optional_filled},
    {"test_optional_reset",      test_optional_reset},
};

int run_libfk_smart_pointer_tests() {
    return run_tests("LibFK Smart Pointers", smart_ptr_tests,
                     sizeof(smart_ptr_tests) / sizeof(smart_ptr_tests[0]));
}
