#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>
#include <stdio.h>

static void test_strcat_basic() {
  char dest[20] = "hello ";
  const char *src = "world";
  strcat(dest, src);
  unsigned char expected[] = {'h', 'e', 'l', 'l', 'o', ' ',
                              'w', 'o', 'r', 'l', 'd', '\0'};
  check_result((unsigned char *)dest, expected, 12, "strcat_basic");
}

void test_strcat() {
  test_strcat_basic();
  printf("All strcat unit tests finished.\n");
}
