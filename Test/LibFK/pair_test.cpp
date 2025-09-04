#include <LibFK/pair.h>
#include <Tests/LibFK/pair_test.h>
#include <Tests/test_runner.h>

static void test_pair_basic() {
  Pair<int, int> p(1, 2);
  check_pair_int(p.first, p.second, 1, 2, "pair_basic");
}

static void test_pair_copy() {
  Pair<int, int> orig(3, 4);
  Pair<int, int> copy = orig;
  check_pair_int(copy.first, copy.second, 3, 4, "pair_copy");
}

static void test_pair_assignment() {
  Pair<int, int> a(5, 6);
  Pair<int, int> b;
  b = a;
  check_pair_int(b.first, b.second, 5, 6, "pair_assignment");
}

static void test_pair_equality() {
  Pair<int, int> a(7, 8);
  Pair<int, int> b(7, 8);
  Pair<int, int> c(8, 7);

  if (a == b) {
    printf("%s[PASS]%s pair_equality (a==b)\n", COLOR_GREEN, COLOR_RESET);
  } else {
    printf("%s[FAIL]%s pair_equality (a==b)\n", COLOR_RED, COLOR_RESET);
  }

  if (a != c) {
    printf("%s[PASS]%s pair_inequality (a!=c)\n", COLOR_GREEN, COLOR_RESET);
  } else {
    printf("%s[FAIL]%s pair_inequality (a!=c)\n", COLOR_RED, COLOR_RESET);
  }
}

extern "C" void test_pair() {
  test_pair_basic();
  test_pair_copy();
  test_pair_assignment();
  test_pair_equality();
  printf("All pair unit tests finished.\n");
}
