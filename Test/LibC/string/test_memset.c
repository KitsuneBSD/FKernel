#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>

static void test_memset_basic() {
  unsigned char buffer[10];
  memset(buffer, 'A', 10);

  unsigned char expected[10];
  for (int i = 0; i < 10; i++)
    expected[i] = 'A';

  check_result(buffer, expected, 10, "memset_basic");
}

static void test_memset_zero_length() {
  unsigned char buffer[5] = {'x', 'y', 'z', 'w', 'v'};
  memset(buffer, 'B', 0);

  unsigned char expected[5] = {'x', 'y', 'z', 'w', 'v'};
  check_result(buffer, expected, 5, "memset_zero_length");
}

static void test_memset_partial() {
  unsigned char buffer[6] = {'1', '2', '3', '4', '5', '6'};
  memset(buffer, 'C', 3);

  unsigned char expected[6] = {'C', 'C', 'C', '4', '5', '6'};
  check_result(buffer, expected, 6, "memset_partial");
}

static void test_memset_value_truncate() {
  unsigned char buffer[4];
  memset(buffer, 300, 4); // 300 % 256 = 44

  unsigned char expected[4] = {44, 44, 44, 44};
  check_result(buffer, expected, 4, "memset_value_truncate");
}

void test_memset() {
  test_memset_basic();
  test_memset_zero_length();
  test_memset_partial();
  test_memset_value_truncate();

  printf("All memset unit tests finished.\n");
}
