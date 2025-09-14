#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_strchr_basic() {
  const char *s = "hello world";
  check_result_ptr(strchr(s, 'o'), s + 4, "strchr_basic");
}

static void test_strchr_not_found() {
  const char *s = "hello";
  check_result_ptr(strchr(s, 'x'), NULL, "strchr_not_found");
}

static void test_strchr_null_terminator() {
  const char *s = "abc";
  check_result_ptr(strchr(s, '\0'), s + 3, "strchr_null_terminator");
}

void test_strchr() {
  test_strchr_basic();
  test_strchr_not_found();
  test_strchr_null_terminator();
  printf("All strchr unit tests finished.\n");
}
