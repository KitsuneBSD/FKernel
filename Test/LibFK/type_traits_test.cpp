#include <LibFK/type_traits.h>
#include <Tests/LibFK/type_traits_test.h>
#include <Tests/test_runner.h>

template <typename T>
typename enable_if<is_integral<T>::value, int>::type only_integral(T) {
  return 1;
}

template <typename T>
typename enable_if<!is_integral<T>::value, int>::type only_integral(T) {
  return 0;
}

static void test_enable_if() {
  int r1 = only_integral(42);   // int -> integral
  int r2 = only_integral(3.14); // double -> não integral
  check_int(r1, 1, "enable_if_integral");
  check_int(r2, 0, "enable_if_non_integral");
}

static void test_is_same() {
  check_bool((is_same<int, int>::value), true, "is_same_int_int");
  check_bool((is_same<int, unsigned int>::value), false, "is_same_int_uint");
  check_bool((is_same<int, float>::value), false, "is_same_int_float");
}

static void test_remove_const() {
  check_bool((is_same<remove_const<const int>::type, int>::value), true,
             "remove_const_const_int");
  check_bool((is_same<remove_const<int>::type, int>::value), true,
             "remove_const_int");
}

static void test_is_integral() {
  check_bool((is_integral<int>::value), true, "is_integral_int");
  check_bool((is_integral<unsigned long>::value), true, "is_integral_ulong");
  check_bool((is_integral<float>::value), false, "is_integral_float");
  check_bool((is_integral<double>::value), false, "is_integral_double");
}

static void test_remove_references() {
  check_bool((is_same<remove_reference<int &>::type, int>::value), true,
             "remove_reference_lvalue");
  check_bool((is_same<remove_reference<int &&>::type, int>::value), true,
             "remove_reference_rvalue");
  check_bool((is_same<remove_reference<int>::type, int>::value), true,
             "remove_reference_plain");
}

extern "C" void test_type_traits() {
  test_enable_if();
  test_is_same();
  test_remove_const();
  test_is_integral();
  test_remove_references();
  printf("All type_traits unit tests finished.\n");
}
