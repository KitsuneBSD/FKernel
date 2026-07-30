#include <tests/test_framework.h>
#include <LibFK/Types/types.h>
#include <LibFK/Text/fixed_string.h>

static const char* test_default() {
  fk::text::fixed_string<32> s;
  TEST_ASSERT(s.c_str()[0] == '\0', "empty string");
  TEST_ASSERT(s.size() == 0, "default size is 0");
  return nullptr;
}

static const char* test_construct_cstr() {
  fk::text::fixed_string<32> s("hello");
  TEST_ASSERT_EQ(s.size(), (size_t)5, "size after cstr construction");
  TEST_ASSERT(s.c_str()[0] == 'h', "first char");
  TEST_ASSERT(s.c_str()[4] == 'o', "last char");
  TEST_ASSERT(s.c_str()[5] == '\0', "null terminated");
  return nullptr;
}

static const char* test_assign() {
  fk::text::fixed_string<32> s;
  s = "abc";
  TEST_ASSERT_EQ(s.size(), (size_t)3, "assign size");
  s = "abcdefghijklmnopqrstuvwxyz";
  TEST_ASSERT(s.size() <= 32, "bounded by N");
  return nullptr;
}

static const char* test_append() {
  fk::text::fixed_string<64> s("foo");
  s.append("_bar");
  TEST_ASSERT_EQ(s.size(), (size_t)7, "append size");
  return nullptr;
}

static const char* test_clear() {
  fk::text::fixed_string<32> s("test");
  s.clear();
  TEST_ASSERT(s.size() == 0, "clear sets size to 0");
  TEST_ASSERT(s.c_str()[0] == '\0', "clear null terminates");
  return nullptr;
}

static const char* test_push_back() {
  fk::text::fixed_string<32> s;
  s.push_back('A');
  s.push_back('B');
  TEST_ASSERT_EQ(s.size(), (size_t)2, "push_back size");
  TEST_ASSERT(s.c_str()[0] == 'A', "first pushed");
  TEST_ASSERT(s.c_str()[1] == 'B', "second pushed");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"default construction", test_default},
  {"cstr construction", test_construct_cstr},
  {"assign", test_assign},
  {"append", test_append},
  {"clear", test_clear},
  {"push_back", test_push_back},
};

int run_libfk_fixed_string_tests() {
  return run_tests("FixedString", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
