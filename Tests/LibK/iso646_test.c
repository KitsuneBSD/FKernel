#include "../../Include/LibK/iso646.h"
#include "Include/tests.h"

void test_and_operator() {
  ASSERT_TRUE((1 and 1) == 1);
  ASSERT_TRUE((1 and 0) == 0);
}

void test_or_operator() {
  ASSERT_TRUE((1 or 0) == 1);
  ASSERT_TRUE((0 or 0) == 0);
}

void test_not_operator() {
  ASSERT_TRUE((not 1) == 0);
  ASSERT_TRUE((not 0) == 1);
}

void test_xor_operator() {
  ASSERT_TRUE((1 xor 0) == 1);
  ASSERT_TRUE((1 xor 1) == 0);
}

int main() {
  RUN_TEST(test_and_operator);
  RUN_TEST(test_or_operator);
  RUN_TEST(test_not_operator);
  RUN_TEST(test_xor_operator);
  print_test_summary();
  return tests_failed;
}
