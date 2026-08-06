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
// is_constructible / is_convertible / is_assignable families
// ---------------------------------------------------------------------------

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

struct NonMovable {
    NonMovable() = default;
    NonMovable(const NonMovable&) = default;
    NonMovable& operator=(const NonMovable&) = default;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

static const char* test_is_constructible() {
    TEST_ASSERT(is_constructible_v<int>,            "int default-constructible");
    constexpr bool int_from_int = is_constructible_v<int, int>;
    TEST_ASSERT(int_from_int,                       "int constructible from int");
    constexpr bool int_from_base = is_constructible_v<int, TestBase>;
    TEST_ASSERT(!int_from_base,                     "int not constructible from TestBase");
    TEST_ASSERT(is_constructible_v<TestBase>,       "TestBase default-constructible");
    return NULL;
}

static const char* test_is_default_constructible() {
    TEST_ASSERT(is_default_constructible_v<int>,      "int default-constructible");
    TEST_ASSERT(is_default_constructible_v<TestBase>, "TestBase default-constructible");
    return NULL;
}

static const char* test_is_copy_constructible() {
    TEST_ASSERT(is_copy_constructible_v<int>,           "int copy-constructible");
    TEST_ASSERT(is_copy_constructible_v<TestBase>,      "TestBase copy-constructible");
    TEST_ASSERT(!is_copy_constructible_v<NonCopyable>,  "NonCopyable not copy-constructible");
    TEST_ASSERT(is_copy_constructible_v<NonMovable>,    "NonMovable is copy-constructible");
    return NULL;
}

static const char* test_is_move_constructible() {
    TEST_ASSERT(is_move_constructible_v<int>,          "int move-constructible");
    TEST_ASSERT(is_move_constructible_v<NonCopyable>,  "NonCopyable move-constructible");
    TEST_ASSERT(!is_move_constructible_v<NonMovable>,  "NonMovable not move-constructible");
    return NULL;
}

static const char* test_is_convertible() {
    TEST_ASSERT((is_convertible_v<int, long>),       "int converts to long");
    TEST_ASSERT((is_convertible_v<int, float>),      "int converts to float");
    TEST_ASSERT((!is_convertible_v<TestBase, int>),  "TestBase not convertible to int");
    TEST_ASSERT((is_convertible_v<TestDerived*, TestBase*>), "Derived* to Base*");
    return NULL;
}

static const char* test_is_copy_assignable() {
    TEST_ASSERT(is_copy_assignable_v<int>,           "int copy-assignable");
    TEST_ASSERT(!is_copy_assignable_v<NonCopyable>,  "NonCopyable not copy-assignable");
    TEST_ASSERT(is_copy_assignable_v<NonMovable>,    "NonMovable copy-assignable");
    return NULL;
}

static const char* test_is_move_assignable() {
    TEST_ASSERT(is_move_assignable_v<int>,           "int move-assignable");
    TEST_ASSERT(is_move_assignable_v<NonCopyable>,   "NonCopyable move-assignable");
    TEST_ASSERT(!is_move_assignable_v<NonMovable>,   "NonMovable not move-assignable");
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
    { "is_constructible",               test_is_constructible              },
    { "is_default_constructible",       test_is_default_constructible      },
    { "is_copy_constructible",          test_is_copy_constructible         },
    { "is_move_constructible",          test_is_move_constructible         },
    { "is_convertible",                 test_is_convertible                },
    { "is_copy_assignable",             test_is_copy_assignable            },
    { "is_move_assignable",             test_is_move_assignable            },
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
