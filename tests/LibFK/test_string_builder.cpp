#include <tests/test_framework.h>
#include <LibFK/Text/string_builder.h>

using namespace fk::text;

static const char* test_sb_append_str() {
    StringBuilder sb;
    sb.append("hello");
    sb.append(" world");
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("hello world", result.c_str(), "append strings");
    return NULL;
}

static const char* test_sb_append_char() {
    StringBuilder sb;
    sb.append('A');
    sb.append('B');
    sb.append('C');
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("ABC", result.c_str(), "append chars");
    return NULL;
}

static const char* test_sb_append_decimal() {
    StringBuilder sb;
    sb.append_decimal(42);
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("42", result.c_str(), "append decimal");
    return NULL;
}

static const char* test_sb_append_hex() {
    StringBuilder sb;
    sb.append_hex(0xDEAD);
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("0xdead", result.c_str(), "append hex with prefix");
    return NULL;
}

static const char* test_sb_clear() {
    StringBuilder sb;
    sb.append("something");
    sb.clear();
    TEST_ASSERT_EQ(0u, sb.length(), "length should be 0 after clear");
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("", result.c_str(), "empty after clear");
    return NULL;
}

static const char* test_sb_mixed() {
    StringBuilder sb;
    sb.append("val=");
    sb.append_decimal(100);
    sb.append(" hex=");
    sb.append_hex(0xFF, false);
    String result = sb.to_string();
    TEST_ASSERT_STR_EQ("val=100 hex=ff", result.c_str(), "mixed append");
    return NULL;
}

static const test_case_t tests[] = {
    {"sb_append_str", test_sb_append_str},
    {"sb_append_char", test_sb_append_char},
    {"sb_append_decimal", test_sb_append_decimal},
    {"sb_append_hex", test_sb_append_hex},
    {"sb_clear", test_sb_clear},
    {"sb_mixed", test_sb_mixed},
};

int run_libfk_stringbuilder_tests() {
    return run_tests("LibFK StringBuilder", tests, sizeof(tests) / sizeof(tests[0]));
}
