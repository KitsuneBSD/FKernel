#include <LibFK/array.h>
#include <LibFK/fixed_string.h>
#include <LibFK/span.h>
#include <LibFK/static_vector.h>
#include <Tests/LibFK/container_test.h>
#include <Tests/test_runner.h>

static void test_array_basic() {
  array<int, 3> a = {1, 2, 3};
  check_int(a[0], 1, "array_index0");
  check_int(a[1], 2, "array_index1");
  check_int(a[2], 3, "array_index2");
  check_int(a.size(), 3, "array_size");
}

static void test_static_vector_basic() {
  static_vector<int, 3> v;
  check_int(v.size(), 0, "static_vector_empty_size");
  bool ok = v.push_back(10);
  check_bool(ok, true, "static_vector_push_back_ok");
  check_int(v.size(), 1, "static_vector_size_after_push");
  check_int(v[0], 10, "static_vector_access");
  v.push_back(20);
  v.push_back(30);
  ok = v.push_back(40); // deve falhar, capacidade = 3
  check_bool(ok, false, "static_vector_push_back_full");
}

static void test_span_basic() {
  int arr[3] = {1, 2, 3};
  span<int> s(arr, 3);
  check_int(s[0], 1, "span_index0");
  check_int(s[1], 2, "span_index1");
  check_int(s[2], 3, "span_index2");
  check_int(s.size(), 3, "span_size");
}

static void test_fixed_string_basic() {
  fixed_string<5> fs;
  bool ok = fs.append("abc");
  check_bool(ok, true, "fixed_string_append_ok");
  check_int(fs.size(), 3, "fixed_string_size_after_append");
  check_int(fs[0], 'a', "fixed_string_index0");
  check_int(fs[1], 'b', "fixed_string_index1");
  check_int(fs[2], 'c', "fixed_string_index2");

  ok = fs.append("de"); // deve caber
  check_bool(ok, true, "fixed_string_append_fill");
  ok = fs.append("f"); // deve falhar, capacidade = 5
  check_bool(ok, false, "fixed_string_append_overflow");

  char *cstr = fs.c_str();
  check_result_ptr(cstr, fs.buffer, "fixed_string_c_str");
  check_int(cstr[0], 'a', "fixed_string_c_str0");
  check_int(cstr[4], 'e', "fixed_string_c_str4"); // último char
}

extern "C" void test_containers() {
  test_array_basic();
  test_static_vector_basic();
  test_span_basic();
  test_fixed_string_basic();
  printf("All containers unit tests finished.\n");
}
