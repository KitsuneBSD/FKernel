#include <tests/test_framework.h>
#include <LibFK/Utilities/tuple.h>
#include <LibFK/Traits/type_traits.h>

using namespace fk::utilities;

static const char* test_tuple_default_construct() {
    Tuple<int, int, int> t;
    (void)t;
    return NULL;
}

static const char* test_tuple_get_set() {
    Tuple<int, float, bool> t(42, 3.0f, true);
    TEST_ASSERT_EQ(42, get<0>(t), "first element");
    TEST_ASSERT(get<2>(t), "third element should be true");
    return NULL;
}

static const char* test_tuple_size() {
    using T3 = Tuple<int, char, double>;
    using T1 = Tuple<int>;
    using T0 = Tuple<>;
    TEST_ASSERT_EQ(3, (int)tuple_size_v<T3>, "tuple size 3");
    TEST_ASSERT_EQ(1, (int)tuple_size_v<T1>, "tuple size 1");
    TEST_ASSERT_EQ(0, (int)tuple_size_v<T0>, "tuple size 0");
    return NULL;
}

static const char* test_tuple_make_tuple() {
    auto t = make_tuple(10, 20, 30);
    TEST_ASSERT_EQ(10, get<0>(t), "get<0>");
    TEST_ASSERT_EQ(20, get<1>(t), "get<1>");
    TEST_ASSERT_EQ(30, get<2>(t), "get<2>");
    return NULL;
}

static const char* test_tuple_move() {
    Tuple<int, int> a(1, 2);
    Tuple<int, int> b(fk::types::move(a));
    TEST_ASSERT_EQ(1, get<0>(b), "moved get<0>");
    TEST_ASSERT_EQ(2, get<1>(b), "moved get<1>");
    return NULL;
}

static const char* test_tuple_copy() {
    Tuple<int, int> a(7, 8);
    Tuple<int, int> b(a);
    TEST_ASSERT_EQ(7, get<0>(b), "copied get<0>");
    TEST_ASSERT_EQ(8, get<1>(b), "copied get<1>");
    get<0>(b) = 99;
    TEST_ASSERT_EQ(7, get<0>(a), "original unmodified");
    return NULL;
}

static const char* test_tuple_element_type() {
    using Pair = Tuple<int, char>;
    static_assert(fk::traits::is_same_v<tuple_element_t<0, Pair>, int>, "first is int");
    static_assert(fk::traits::is_same_v<tuple_element_t<1, Pair>, char>, "second is char");
    return NULL;
}

static const char* test_tuple_single() {
    Tuple<int> t(42);
    TEST_ASSERT_EQ(42, get<0>(t), "single element");
    return NULL;
}

static const test_case_t s_tuple_tests[] = {
    {"test_tuple_default_construct", test_tuple_default_construct},
    {"test_tuple_get_set",           test_tuple_get_set},
    {"test_tuple_size",              test_tuple_size},
    {"test_tuple_make_tuple",        test_tuple_make_tuple},
    {"test_tuple_move",              test_tuple_move},
    {"test_tuple_copy",              test_tuple_copy},
    {"test_tuple_element_type",      test_tuple_element_type},
    {"test_tuple_single",            test_tuple_single},
};

int run_libfk_tuple_tests() {
    return run_tests("LibFK Tuple",
                     s_tuple_tests,
                     sizeof(s_tuple_tests) / sizeof(s_tuple_tests[0]));
}
