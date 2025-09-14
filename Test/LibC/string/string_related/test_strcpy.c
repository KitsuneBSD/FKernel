#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_strcpy_basic() {
  char dest[16];
  const char *src = "hello";
  strcpy(dest, src);

  unsigned char expected[] = {'h', 'e', 'l', 'l', 'o', '\0'};
  check_result((unsigned char *)dest, expected, 6, "strcpy_basic");
}

static void test_strcpy_empty() {
  char dest[4] = {'x', 'y', 'z', 'w'};
  const char *src = "";
  strcpy(dest, src);

  unsigned char expected[] = {'\0', 'y', 'z', 'w'};
  check_result((unsigned char *)dest, expected, 4, "strcpy_empty");
}

static void test_strcpy_with_space() {
  char dest[16];
  const char *src = "a b c";
  strcpy(dest, src);

  unsigned char expected[] = {'a', ' ', 'b', ' ', 'c', '\0'};
  check_result((unsigned char *)dest, expected, 6, "strcpy_with_space");
}

static void test_strcpy_with_newline() {
  char dest[16];
  const char *src = "line\nbreak";
  strcpy(dest, src);

  unsigned char expected[] = {'l', 'i', 'n', 'e', '\n', 'b',
                              'r', 'e', 'a', 'k', '\0'};
  check_result((unsigned char *)dest, expected, 11, "strcpy_with_newline");
}

void test_strcpy() {
  test_strcpy_basic();
  test_strcpy_empty();
  test_strcpy_with_space();
  test_strcpy_with_newline();

  printf("All strcpy unit tests finished.\n");
}
