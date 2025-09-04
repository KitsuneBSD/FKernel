#include <LibFK/optional.h>
#include <Tests/LibFK/optional_test.h>
#include <Tests/test_runner.h>
#include <assert.h>

static void test_optional_empty() {
  optional<int> o;
  check_bool(o.has_value(), false, "optional_empty");
}

static void test_optional_with_value() {
  optional<int> o(42);
  check_bool(o.has_value(), true, "optional_with_value");
  check_int(o.value(), 42, "optional_value_check");
}

static void test_optional_copy() {
  optional<int> o1(7);
  optional<int> o2 = o1;
  check_bool(o2.has_value(), true, "optional_copy_has_value");
  check_int(o2.value(), 7, "optional_copy_value");
}

static void test_optional_assignment() {
  optional<int> o1(10);
  optional<int> o2;
  o2 = o1;
  check_bool(o2.has_value(), true, "optional_assignment_has_value");
  check_int(o2.value(), 10, "optional_assignment_value");
}

static void test_optional_reset() {
  optional<int> o(99);
  o.reset();
  check_bool(o.has_value(), false, "optional_reset");
}

static void test_optional_default_constructed() {
  optional<int> opt;
  assert(!opt.has_value());
}

static void test_optional_construct_with_value() {
  optional<int> opt(42);
  assert(opt.has_value());
  assert(opt.value() == 42);
}

static void test_optional_copy_construct() {
  optional<int> a(123);
  optional<int> b(a);
  assert(b.has_value());
  assert(b.value() == 123);
}

static void test_optional_reassign() {
  optional<int> a(5);
  a = optional<int>(99);
  assert(a.has_value());
  assert(a.value() == 99);
}

extern "C" void test_optional() {
  test_optional_empty();
  test_optional_with_value();
  test_optional_copy();
  test_optional_assignment();
  test_optional_reset();
  test_optional_default_constructed();
  test_optional_construct_with_value();
  test_optional_copy_construct();
  test_optional_reassign();

  printf("All optional unit tests finished.\n");
}
