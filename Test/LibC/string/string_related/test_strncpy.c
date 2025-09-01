#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>
#include <stdio.h>

static void test_strncpy_basic() {
  char dest[10] = {0};
  const char *src = "abc";
  strncpy(dest, src, 5);
  unsigned char expected[] = {'a', 'b', 'c', '\0', '\0'};
  check_result((unsigned char *)dest, expected, 5, "strncpy_basic");
}

static void test_strncpy_truncate() {
  char dest[4];
  const char *src = "abcdef";
  strncpy(dest, src, 4);
  unsigned char expected[] = {'a', 'b', 'c', 'd'};
  check_result((unsigned char *)dest, expected, 4, "strncpy_truncate");
}

void test_strncpy() {
  test_strncpy_basic();
  test_strncpy_truncate();
  printf("All strncpy unit tests finished.\n");
}
