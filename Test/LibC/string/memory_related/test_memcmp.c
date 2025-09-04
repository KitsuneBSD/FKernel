#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static unsigned char memcmp_result_to_byte(int cmp) {
  if (cmp == 0)
    return 0;
  return cmp > 0 ? 1 : 255;
}

static void test_memcmp_equal() {
  unsigned char a[5] = {'a', 'b', 'c', 'd', 'e'};
  unsigned char b[5] = {'a', 'b', 'c', 'd', 'e'};

  int cmp = memcmp(a, b, 5);
  unsigned char actual[1] = {memcmp_result_to_byte(cmp)};
  unsigned char expected[1] = {0};

  check_result(actual, expected, 1, "memcmp_equal");
}

static void test_memcmp_first_greater() {
  unsigned char a[5] = {'b', 'b', 'c', 'd', 'e'};
  unsigned char b[5] = {'a', 'b', 'c', 'd', 'e'};

  int cmp = memcmp(a, b, 5);
  unsigned char actual[1] = {memcmp_result_to_byte(cmp)};
  unsigned char expected[1] = {1};

  check_result(actual, expected, 1, "memcmp_first_greater");
}

static void test_memcmp_second_greater() {
  unsigned char a[5] = {'a', 'b', 'c', 'd', 'e'};
  unsigned char b[5] = {'b', 'b', 'c', 'd', 'e'};

  int cmp = memcmp(a, b, 5);
  unsigned char actual[1] = {memcmp_result_to_byte(cmp)};
  unsigned char expected[1] = {255};

  check_result(actual, expected, 1, "memcmp_second_greater");
}

static void test_memcmp_partial_difference() {
  unsigned char a[5] = {'a', 'b', 'x', 'd', 'e'};
  unsigned char b[5] = {'a', 'b', 'c', 'd', 'e'};

  int cmp = memcmp(a, b, 5);
  unsigned char actual[1] = {memcmp_result_to_byte(cmp)};
  unsigned char expected[1] = {1}; // 'x' > 'c'

  check_result(actual, expected, 1, "memcmp_partial_difference");
}

void test_memcmp() {
  test_memcmp_equal();
  test_memcmp_first_greater();
  test_memcmp_second_greater();
  test_memcmp_partial_difference();

  printf("All memcmp unit tests finished.\n");
}
