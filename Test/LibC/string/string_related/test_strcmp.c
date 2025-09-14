#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_strcmp_equal() {
  check_result_int(strcmp("hello", "hello"), 0, "strcmp_equal");
}

static void test_strcmp_less() {
  // "abc" < "abd"
  check_result_int(strcmp("abc", "abd"), -1, "strcmp_less");
}

static void test_strcmp_greater() {
  // "abd" > "abc"
  check_result_int(strcmp("abd", "abc"), 1, "strcmp_greater");
}

static void test_strcmp_prefix() {
  // "abc" < "abcd"
  check_result_int(strcmp("abc", "abcd"), -1, "strcmp_prefix");
}

static void test_strcmp_empty_vs_nonempty() {
  // "" < "a"
  check_result_int(strcmp("", "a"), -1, "strcmp_empty_vs_nonempty");
}

static void test_strcmp_nonempty_vs_empty() {
  // "a" > ""
  check_result_int(strcmp("a", ""), 1, "strcmp_nonempty_vs_empty");
}

void test_strcmp() {
  test_strcmp_equal();
  test_strcmp_less();
  test_strcmp_greater();
  test_strcmp_prefix();
  test_strcmp_empty_vs_nonempty();
  test_strcmp_nonempty_vs_empty();

  printf("All strcmp unit tests finished.\n");
}
