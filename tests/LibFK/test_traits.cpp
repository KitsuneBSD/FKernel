#include <tests/test_framework.h>
#include <LibFK/Traits/type_traits.h>

using namespace fk::traits;

// ---------------------------------------------------------------------------
// is_class
// ---------------------------------------------------------------------------

struct TestBase    {};
struct TestDerived : TestBase {};
struct TestUnrelated {};

static const char* test_is_class_class_types() {
    TEST_ASSERT(is_class<TestBase>::value,    "struct should be a class type");
    TEST_ASSERT(is_class<TestDerived>::value, "derived struct should be a class type");
    return NULL;
}

static const char* test_is_class_non_class_types() {
    TEST_ASSERT(!is_class<int>::value,   "int should not be a class type");
    TEST_ASSERT(!is_class<void>::value,  "void should not be a class type");
    TEST_ASSERT(!is_class<float>::value, "float should not be a class type");
    return NULL;
}

// ---------------------------------------------------------------------------
// is_base_of
// ---------------------------------------------------------------------------

static const char* test_is_base_of_direct_inheritance() {
    // Use constexpr local to avoid C macro / template comma conflict
    constexpr bool r = is_base_of<TestBase, TestDerived>::value;
    TEST_ASSERT(r, "TestBase should be base of TestDerived");
    return NULL;
}

static const char* test_is_base_of_reflexive() {
    constexpr bool r = is_base_of<TestBase, TestBase>::value;
    TEST_ASSERT(r, "A class should be its own base (reflexive)");
    return NULL;
}

static const char* test_is_base_of_reversed() {
    constexpr bool r = is_base_of<TestDerived, TestBase>::value;
    TEST_ASSERT(!r, "TestDerived should not be base of TestBase");
    return NULL;
}

static const char* test_is_base_of_unrelated() {
    constexpr bool r = is_base_of<TestBase, TestUnrelated>::value;
    TEST_ASSERT(!r, "TestBase should not be base of TestUnrelated");
    return NULL;
}

static const char* test_is_base_of_non_class_types() {
    // Bug fix: was incorrectly returning true because void* accepts any pointer
    constexpr bool void_int  = is_base_of<void, int>::value;
    constexpr bool int_int   = is_base_of<int,  int>::value;
    constexpr bool void_cls  = is_base_of<void, TestDerived>::value;
    TEST_ASSERT(!void_int, "is_base_of<void, int> must be false");
    TEST_ASSERT(!int_int,  "is_base_of<int,  int> must be false (not a class)");
    TEST_ASSERT(!void_cls, "is_base_of<void, TestDerived> must be false (void not a class)");
    return NULL;
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

static struct { const char* name; const char* (*fn)(); } s_tests[] = {
    { "is_class: class types",          test_is_class_class_types          },
    { "is_class: non-class types",      test_is_class_non_class_types      },
    { "is_base_of: direct inheritance", test_is_base_of_direct_inheritance },
    { "is_base_of: reflexive",          test_is_base_of_reflexive          },
    { "is_base_of: reversed",           test_is_base_of_reversed           },
    { "is_base_of: unrelated classes",  test_is_base_of_unrelated          },
    { "is_base_of: non-class types",    test_is_base_of_non_class_types    },
};

int run_libfk_traits_tests() {
    int failed = 0;
    TEST_LOG("\n=== LibFK Traits Tests ===\n");
    for (auto& t : s_tests) {
        const char* result = t.fn();
        if (result) {
            TEST_LOG("  [FAIL] %s: %s\n", t.name, result);
            ++failed;
        } else {
            TEST_LOG("  [PASS] %s\n", t.name);
        }
    }
    return failed;
}
