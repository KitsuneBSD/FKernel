#include <tests/test_framework.h>
#include <LibFK/Text/string.h>

using namespace fk::text;

static const char* test_string_basic_ctor() {
    String s("hello");
    TEST_ASSERT_EQ(5, (int)s.length(), "Length should be 5");
    TEST_ASSERT_STR_EQ("hello", s.c_str(), "Content should be 'hello'");
    return NULL;
}

static const char* test_string_empty() {
    String s;
    TEST_ASSERT(s.is_empty(), "Default-constructed String should be empty");
    TEST_ASSERT_EQ(0, (int)s.length(), "Length should be 0");
    return NULL;
}

static const char* test_string_append() {
    String s("foo");
    s.append("bar");
    TEST_ASSERT_STR_EQ("foobar", s.c_str(), "Appended string should be 'foobar'");
    return NULL;
}

static const char* test_string_operator_plus() {
    String a("hello");
    String b(" world");
    String c = a + b;
    TEST_ASSERT_STR_EQ("hello world", c.c_str(), "Concatenation should produce 'hello world'");
    return NULL;
}

static const char* test_string_starts_with() {
    String s("fkernel");
    TEST_ASSERT(s.starts_with("fker"), "should start with 'fker'");
    TEST_ASSERT(!s.starts_with("kernel"), "should not start with 'kernel'");
    return NULL;
}

static const char* test_string_ends_with() {
    String s("fkernel");
    TEST_ASSERT(s.ends_with("nel"), "should end with 'nel'");
    TEST_ASSERT(!s.ends_with("fker"), "should not end with 'fker'");
    return NULL;
}

static const char* test_string_contains() {
    String s("the quick brown fox");
    TEST_ASSERT(s.contains("quick"), "should contain 'quick'");
    TEST_ASSERT(!s.contains("slow"), "should not contain 'slow'");
    return NULL;
}

static const char* test_string_find() {
    String s("abcabc");
    TEST_ASSERT_EQ(0, (int)s.find('a'), "find 'a' from start");
    TEST_ASSERT_EQ(3, (int)s.find('a', 1), "find 'a' after pos 0");
    TEST_ASSERT_EQ((int)String::NPOS, (int)s.find('z'), "find non-existent char");
    return NULL;
}

static const char* test_string_rfind() {
    String s("abcabc");
    TEST_ASSERT_EQ(3, (int)s.rfind('a'), "rfind 'a' returns last occurrence");
    TEST_ASSERT_EQ((int)String::NPOS, (int)s.rfind('z'), "rfind non-existent char");
    return NULL;
}

static const char* test_string_substr() {
    String s("hello world");
    String sub = s.substr(6, 5);
    TEST_ASSERT_STR_EQ("world", sub.c_str(), "substr should be 'world'");
    return NULL;
}

static const char* test_string_to_upper_lower() {
    String s("Hello World");
    String upper = s.to_upper();
    String lower = s.to_lower();
    TEST_ASSERT_STR_EQ("HELLO WORLD", upper.c_str(), "to_upper should produce 'HELLO WORLD'");
    TEST_ASSERT_STR_EQ("hello world", lower.c_str(), "to_lower should produce 'hello world'");
    return NULL;
}

static const char* test_string_operator_eq() {
    String a("test");
    String b("test");
    String c("other");
    TEST_ASSERT(a == b, "Equal strings should compare equal");
    TEST_ASSERT(!(a == c), "Different strings should not compare equal");
    TEST_ASSERT(a != c, "Different strings should compare not-equal");
    return NULL;
}

static const test_case_t s_text_tests[] = {
    {"test_string_basic_ctor",        test_string_basic_ctor},
    {"test_string_empty",             test_string_empty},
    {"test_string_append",            test_string_append},
    {"test_string_operator_plus",     test_string_operator_plus},
    {"test_string_starts_with",       test_string_starts_with},
    {"test_string_ends_with",         test_string_ends_with},
    {"test_string_contains",          test_string_contains},
    {"test_string_find",              test_string_find},
    {"test_string_rfind",             test_string_rfind},
    {"test_string_substr",            test_string_substr},
    {"test_string_to_upper_lower",    test_string_to_upper_lower},
    {"test_string_operator_eq",       test_string_operator_eq},
};

int run_libfk_text_tests() {
    return run_tests("LibFK Text (String)",
                     s_text_tests,
                     sizeof(s_text_tests) / sizeof(s_text_tests[0]));
}
