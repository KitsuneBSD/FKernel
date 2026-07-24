#include <tests/test_framework.h>
#include <LibFK/Text/string_view.h>

using namespace fk::text;

static const char* test_sv_default_ctor() {
    StringView sv;
    TEST_ASSERT(sv.empty(), "default StringView should be empty");
    TEST_ASSERT_EQ(0, (int)sv.size(), "default StringView size should be 0");
    return NULL;
}

static const char* test_sv_from_cstring() {
    StringView sv("hello");
    TEST_ASSERT_EQ(5, (int)sv.size(), "size of 'hello' should be 5");
    TEST_ASSERT(!sv.empty(), "non-empty StringView should not be empty");
    TEST_ASSERT_EQ('h', sv[0], "first char should be 'h'");
    TEST_ASSERT_EQ('o', sv[4], "last char should be 'o'");
    return NULL;
}

static const char* test_sv_from_ptr_and_len() {
    const char* data = "fkernel";
    StringView sv(data + 1, 3);
    TEST_ASSERT_EQ(3, (int)sv.size(), "size should be 3");
    TEST_ASSERT_EQ('k', sv[0], "first char should be 'k'");
    TEST_ASSERT_EQ('r', sv[2], "third char should be 'r'");
    return NULL;
}

static const char* test_sv_equality() {
    StringView a("hello");
    StringView b("hello");
    StringView c("world");
    TEST_ASSERT(a == b, "equal strings should compare equal");
    TEST_ASSERT(!(a == c), "different strings should not compare equal");
    TEST_ASSERT(a != c, "different strings should compare not-equal");
    return NULL;
}

static const char* test_sv_ordering() {
    StringView a("abc");
    StringView b("abd");
    StringView c("ab");
    TEST_ASSERT(a < b, "'abc' < 'abd'");
    TEST_ASSERT(c < a, "'ab' < 'abc'");
    TEST_ASSERT(b > a, "'abd' > 'abc'");
    TEST_ASSERT(a <= a, "'abc' <= 'abc'");
    TEST_ASSERT(a >= a, "'abc' >= 'abc'");
    return NULL;
}

static const char* test_sv_substr() {
    StringView sv("hello world");
    StringView sub = sv.substr(6, 5);
    TEST_ASSERT(sub == StringView("world"), "substr should be 'world'");
    TEST_ASSERT_EQ(5, (int)sub.size(), "substr size should be 5");
    return NULL;
}

static const char* test_sv_substr_past_end() {
    StringView sv("hello");
    StringView sub = sv.substr(2);
    TEST_ASSERT(sub == StringView("llo"), "substr to end should be 'llo'");
    return NULL;
}

static const char* test_sv_find_char() {
    StringView sv("abcabc");
    TEST_ASSERT_EQ(0, (int)sv.find('a'), "find 'a' from start");
    TEST_ASSERT_EQ(3, (int)sv.find('a', 1), "find 'a' after pos 0");
    TEST_ASSERT_EQ((int)StringView::NPOS, (int)sv.find('z'), "find non-existent char");
    return NULL;
}

static const char* test_sv_rfind_char() {
    StringView sv("abcabc");
    TEST_ASSERT_EQ(3, (int)sv.rfind('a'), "rfind 'a' returns last occurrence");
    TEST_ASSERT_EQ((int)StringView::NPOS, (int)sv.rfind('z'), "rfind non-existent char");
    return NULL;
}

static const char* test_sv_starts_with() {
    StringView sv("fkernel");
    TEST_ASSERT(sv.starts_with(StringView("fker")), "should start with 'fker'");
    TEST_ASSERT(!sv.starts_with(StringView("kernel")), "should not start with 'kernel'");
    TEST_ASSERT(sv.starts_with('f'), "should start with 'f'");
    TEST_ASSERT(!sv.starts_with('k'), "should not start with 'k'");
    return NULL;
}

static const char* test_sv_ends_with() {
    StringView sv("fkernel");
    TEST_ASSERT(sv.ends_with(StringView("nel")), "should end with 'nel'");
    TEST_ASSERT(!sv.ends_with(StringView("fker")), "should not end with 'fker'");
    TEST_ASSERT(sv.ends_with('l'), "should end with 'l'");
    TEST_ASSERT(!sv.ends_with('e'), "should not end with 'e'");
    return NULL;
}

static const char* test_sv_remove_prefix() {
    StringView sv("hello world");
    sv.remove_prefix(6);
    TEST_ASSERT(sv == StringView("world"), "after remove_prefix(6) should be 'world'");
    TEST_ASSERT_EQ(5, (int)sv.size(), "size after remove_prefix should be 5");
    return NULL;
}

static const char* test_sv_remove_suffix() {
    StringView sv("hello world");
    sv.remove_suffix(6);
    TEST_ASSERT(sv == StringView("hello"), "after remove_suffix(6) should be 'hello'");
    TEST_ASSERT_EQ(5, (int)sv.size(), "size after remove_suffix should be 5");
    return NULL;
}

static const char* test_sv_front_back() {
    StringView sv("kernel");
    TEST_ASSERT_EQ('k', sv.front(), "front() should be 'k'");
    TEST_ASSERT_EQ('l', sv.back(), "back() should be 'l'");
    return NULL;
}

static const char* test_sv_iterators() {
    StringView sv("abc");
    int count = 0;
    for (char c : sv) {
        (void)c;
        count++;
    }
    TEST_ASSERT_EQ(3, count, "range-for should iterate 3 characters");
    return NULL;
}

static const test_case_t s_string_view_tests[] = {
    {"sv_default_ctor",       test_sv_default_ctor},
    {"sv_from_cstring",       test_sv_from_cstring},
    {"sv_from_ptr_and_len",   test_sv_from_ptr_and_len},
    {"sv_equality",           test_sv_equality},
    {"sv_ordering",           test_sv_ordering},
    {"sv_substr",             test_sv_substr},
    {"sv_substr_past_end",    test_sv_substr_past_end},
    {"sv_find_char",          test_sv_find_char},
    {"sv_rfind_char",         test_sv_rfind_char},
    {"sv_starts_with",        test_sv_starts_with},
    {"sv_ends_with",          test_sv_ends_with},
    {"sv_remove_prefix",      test_sv_remove_prefix},
    {"sv_remove_suffix",      test_sv_remove_suffix},
    {"sv_front_back",         test_sv_front_back},
    {"sv_iterators",          test_sv_iterators},
};

int run_libfk_string_view_tests() {
    return run_tests("LibFK StringView",
                     s_string_view_tests,
                     sizeof(s_string_view_tests) / sizeof(s_string_view_tests[0]));
}
