#include <tests/test_framework.h>
#include <LibFK/Memory/optional.h>

using fk::memory::optional;

static const char* test_default_empty() {
  optional<int> o;
  TEST_ASSERT(!o.has_value(), "default has no value");
  TEST_ASSERT(!(bool)o, "bool conversion false when empty");
  return nullptr;
}

static const char* test_construct_value() {
  optional<int> o(42);
  TEST_ASSERT(o.has_value(), "has value after construction");
  TEST_ASSERT((bool)o, "bool conversion true");
  TEST_ASSERT_EQ(o.value(), 42, "value() returns 42");
  TEST_ASSERT_EQ(*o, 42, "operator* returns 42");
  return nullptr;
}

static const char* test_copy_construct() {
  optional<int> a(7);
  optional<int> b(a);
  TEST_ASSERT(b.has_value(), "copy has value");
  TEST_ASSERT_EQ(b.value(), 7, "copy value matches");
  return nullptr;
}

static const char* test_move_construct() {
  optional<int> a(99);
  optional<int> b(static_cast<optional<int>&&>(a));
  TEST_ASSERT(b.has_value(), "move target has value");
  TEST_ASSERT_EQ(b.value(), 99, "move value matches");
  TEST_ASSERT(!a.has_value(), "source empty after move");
  return nullptr;
}

static const char* test_copy_assign() {
  optional<int> a(5);
  optional<int> b;
  b = a;
  TEST_ASSERT(b.has_value(), "copy-assigned has value");
  TEST_ASSERT_EQ(b.value(), 5, "copy-assigned value");
  return nullptr;
}

static const char* test_move_assign() {
  optional<int> a(11);
  optional<int> b;
  b = static_cast<optional<int>&&>(a);
  TEST_ASSERT(b.has_value(), "move-assigned has value");
  TEST_ASSERT_EQ(b.value(), 11, "move-assigned value");
  TEST_ASSERT(!a.has_value(), "source empty after move-assign");
  return nullptr;
}

static const char* test_value_or() {
  optional<int> with_value(3);
  optional<int> empty;
  TEST_ASSERT_EQ(with_value.value_or(99), 3, "value_or returns value when set");
  TEST_ASSERT_EQ(empty.value_or(99), 99, "value_or returns default when empty");
  return nullptr;
}

static const char* test_emplace() {
  optional<int> o;
  o.emplace(77);
  TEST_ASSERT(o.has_value(), "emplace sets value");
  TEST_ASSERT_EQ(o.value(), 77, "emplace value");
  o.emplace(123);
  TEST_ASSERT_EQ(o.value(), 123, "re-emplace value");
  return nullptr;
}

static const char* test_reset() {
  optional<int> o(42);
  o.reset();
  TEST_ASSERT(!o.has_value(), "reset clears value");
  return nullptr;
}

static const char* test_arrow_operator() {
  struct Point { int x; int y; };
  optional<Point> p(Point{3, 4});
  TEST_ASSERT_EQ(p->x, 3, "arrow x");
  TEST_ASSERT_EQ(p->y, 4, "arrow y");
  return nullptr;
}

static const char* test_take() {
  optional<int> o(55);
  int v = o.take();
  TEST_ASSERT_EQ(v, 55, "take returns value");
  TEST_ASSERT(!o.has_value(), "take leaves optional empty");
  return nullptr;
}

static const test_case_t s_tests[] = {
  {"default empty",     test_default_empty},
  {"construct value",   test_construct_value},
  {"copy construct",    test_copy_construct},
  {"move construct",    test_move_construct},
  {"copy assign",       test_copy_assign},
  {"move assign",       test_move_assign},
  {"value_or",          test_value_or},
  {"emplace",           test_emplace},
  {"reset",             test_reset},
  {"arrow operator",    test_arrow_operator},
  {"take",              test_take},
};

int run_libfk_optional_tests() {
  return run_tests("LibFK::Optional", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
