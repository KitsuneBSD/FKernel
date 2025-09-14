#include <LibC/string.h>
#include <Tests/LibC/string_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_memmove_overlap_forward() {
  unsigned char buffer[20];
  for (int i = 0; i < 10; i++)
    buffer[i] = 'a' + i;
  memmove(buffer + 5, buffer, 5);
  unsigned char expected[10] = {'a', 'b', 'c', 'd', 'e',
                                'a', 'b', 'c', 'd', 'e'};
  check_result(buffer, expected, 10, "memmove_overlap_forward");
}

static void test_memmove_overlap_backward() {
  unsigned char buffer[20];
  for (int i = 0; i < 10; i++)
    buffer[i] = '1' + i;
  memmove(buffer, buffer + 3, 5);
  unsigned char expected[10] = {'4', '5', '6', '7', '8',
                                '6', '7', '8', '9', ':'};
  check_result(buffer, expected, 10, "memmove_overlap_backward");
}

static void test_memmove_full_overlap() {
  unsigned char buffer[20];
  for (int i = 0; i < 10; i++)
    buffer[i] = 'a' + i;
  memmove(buffer + 2, buffer, 8);
  unsigned char expected[10] = {'a', 'b', 'a', 'b', 'c',
                                'd', 'e', 'f', 'g', 'h'};
  check_result(buffer, expected, 10, "memmove_full_overlap");
}

static void test_memmove_zero_length() {
  unsigned char buffer[4];
  for (int i = 0; i < 4; i++)
    buffer[i] = 't' + i;
  memmove(buffer + 1, buffer, 0);
  unsigned char expected[4] = {'t', 'u', 'v', 'w'};
  check_result(buffer, expected, 4, "memmove_zero_length");
}

static void test_memmove_self_copy() {
  unsigned char buffer[4];
  for (int i = 0; i < 4; i++)
    buffer[i] = 's' + i;
  memmove(buffer, buffer, 4);
  unsigned char expected[4] = {'s', 't', 'u', 'v'};
  check_result(buffer, expected, 4, "memmove_self_copy");
}

void test_memmove() {
  test_memmove_overlap_forward();
  test_memmove_overlap_backward();
  test_memmove_full_overlap();
  test_memmove_zero_length();
  test_memmove_self_copy();

  printf("All memmove unit tests finished.\n");
}
