#include "../../Include/LibK/stdbool.h"
#include "Include/tests.h"

void test_bool_definitions() {
  ASSERT_EQ(sizeof(bool), 1);
  ASSERT_EQ(true, 1);
  ASSERT_EQ(false, 0);
}

void test_bool_behavior() {
  _Bool a = 0;
  _Bool b = 1;
  _Bool c = 42;
  ASSERT_EQ(a, false);
  ASSERT_EQ(b, true);
  ASSERT_EQ(c, true);
}

void test_bool_operations() {
  bool a = true;
  bool b = false;

  ASSERT_EQ(a && b, false);
  ASSERT_EQ(a || b, true);
  ASSERT_EQ(!a, false);
  ASSERT_EQ(!b, true);
}

int main() {
  RUN_TEST(test_bool_definitions);
  RUN_TEST(test_bool_behavior);
  RUN_TEST(test_bool_operations);

  print_test_summary();
  return tests_failed > 0 ? 1 : 0;
}
