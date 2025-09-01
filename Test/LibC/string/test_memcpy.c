#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_memcpy_basic() {
  unsigned char buffer[10];
  for (int i = 0; i < 10; i++)
    buffer[i] = 'a' + i;

  unsigned char dest[10];
  memcpy(dest, buffer, 10);

  unsigned char expected[10] = {'a', 'b', 'c', 'd', 'e',
                                'f', 'g', 'h', 'i', 'j'};
  check_result(dest, expected, 10, "memcpy_basic");
}

static void test_memcpy_zero_length() {
  unsigned char buffer[4] = {'x', 'y', 'z', 'w'};
  unsigned char dest[4] = {0};

  memcpy(dest, buffer, 0);

  unsigned char expected[4] = {0, 0, 0, 0};
  check_result(dest, expected, 4, "memcpy_zero_length");
}

static void test_memcpy_partial_copy() {
  unsigned char buffer[6] = {'1', '2', '3', '4', '5', '6'};
  unsigned char dest[6] = {0};

  memcpy(dest, buffer, 3);

  unsigned char expected[6] = {'1', '2', '3', 0, 0, 0};
  check_result(dest, expected, 6, "memcpy_partial_copy");
}

void test_memcpy() {
  test_memcpy_basic();
  test_memcpy_zero_length();
  test_memcpy_partial_copy();

  printf("All memcpy unit tests finished.\n");
}
