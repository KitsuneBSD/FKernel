#include "../../Include/LibK/stddef.h"
#include "Include/tests.h"

void test_null_definition() { ASSERT_EQ(NULL, ((void *)0)); }

void test_size_t_definition() {
  size_t x = 10;
  ASSERT_TRUE(x > 0);
}

void test_ptrdiff_t_definition() {
  int arr[10];
  ptrdiff_t diff = &arr[5] - &arr[2];
  ASSERT_TRUE(diff == 3);
}

struct TestStruct {
  char a;
  int b;
  double c;
};

void test_offsetof_macro() {
  ASSERT_EQ(offsetof(struct TestStruct, a), 0);
  ASSERT_TRUE(offsetof(struct TestStruct, b) > 0);
  ASSERT_TRUE(offsetof(struct TestStruct, c) > offsetof(struct TestStruct, b));
}

int main() {
  RUN_TEST(test_null_definition);
  RUN_TEST(test_size_t_definition);
  RUN_TEST(test_ptrdiff_t_definition);
  RUN_TEST(test_offsetof_macro);

  print_test_summary();
  return tests_failed;
}
