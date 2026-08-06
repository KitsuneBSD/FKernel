#pragma once

#include <LibC/stddef.h>

namespace fk {
namespace traits {

/**
 * @brief Conditional enable_if.
 *
 * When B is true, defines ::type as T. Otherwise, ::type is not defined.
 * Useful for SFINAE (Substitution Failure Is Not An Error).
 */
template <bool B, typename T = void> struct enable_if {};

/**
 * @brief Specialization when B is true.
 */
template <typename T> struct enable_if<true, T> {
  using type = T;
};

/**
 * @brief Alias to simplify enable_if usage.
 */
template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

/**
 * @brief Checks if two types are the same.
 */
template <typename T, typename U> struct is_same {
  static constexpr bool value = false;
};

/**
 * @brief Specialization when both types are identical.
 */
template <typename T> struct is_same<T, T> {
  static constexpr bool value = true;
};

/**
 * @brief Alias to directly obtain the boolean value.
 */
template <typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;

/**
 * @brief Removes const qualifier from a type.
 */
template <typename T> struct remove_const {
  using type = T;
};

/**
 * @brief Specialization for const-qualified types.
 */
template <typename T> struct remove_const<const T> {
  using type = T;
};

/**
 * @brief Alias to simplify remove_const usage.
 */
template <typename T> using remove_const_t = typename remove_const<T>::type;

/**
 * @brief Removes reference qualifiers from a type.
 */
template <typename T> struct remove_reference {
  using type = T;
};

template <typename T> struct remove_reference<T &> {
  using type = T;
};

template <typename T> struct remove_reference<T &&> {
  using type = T;
};

/**
 * @brief Alias to simplify remove_reference usage.
 */
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

/**
 * @brief Checks if a type is an integral type.
 */
template <typename T> struct is_integral {
  static constexpr bool value = false;
};

#define FK_DEFINE_INTEGRAL(T)                                                  \
  template <> struct is_integral<T> {                                          \
    static constexpr bool value = true;                                        \
  }

FK_DEFINE_INTEGRAL(bool);
FK_DEFINE_INTEGRAL(char);
FK_DEFINE_INTEGRAL(signed char);
FK_DEFINE_INTEGRAL(unsigned char);
FK_DEFINE_INTEGRAL(short);
FK_DEFINE_INTEGRAL(unsigned short);
FK_DEFINE_INTEGRAL(int);
FK_DEFINE_INTEGRAL(unsigned int);
FK_DEFINE_INTEGRAL(long);
FK_DEFINE_INTEGRAL(unsigned long);
FK_DEFINE_INTEGRAL(long long);
FK_DEFINE_INTEGRAL(unsigned long long);

#undef FK_DEFINE_INTEGRAL

/**
 * @brief Alias to directly obtain the boolean value.
 */
template <typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;

/**
 * @brief Checks if a type is an array type.
 */
template <typename T> struct is_array {
  static constexpr bool value = false;
};

/**
 * @brief Specialization for unbounded array types.
 */
template <typename T> struct is_array<T[]> {
  static constexpr bool value = true;
};

/**
 * @brief Specialization for bounded array types.
 */
template <typename T, size_t N> struct is_array<T[N]> {
  static constexpr bool value = true;
};

/**
 * @brief Alias to directly obtain the boolean value for is_array.
 */
template <typename T> inline constexpr bool is_array_v = is_array<T>::value;

/**
 * @brief Removes the outermost array extent from a type.
 */
template <typename T> struct remove_extent {
  using type = T;
};

/**
 * @brief Specialization for unbounded array types.
 */
template <typename T> struct remove_extent<T[]> {
  using type = T;
};

/**
 * @brief Specialization for bounded array types.
 */
template <typename T, size_t N> struct remove_extent<T[N]> {
  using type = T;
};

/**
 * @brief Alias to simplify remove_extent usage.
 */
template <typename T> using remove_extent_t = typename remove_extent<T>::type;

/**
 * @brief Detects whether T is a class or struct type.
 *
 * Uses pointer-to-member SFINAE: int T::* is only valid for class types.
 */
template <typename T> struct is_class {
private:
  template <typename U> static char  check(int U::*);
  template <typename U> static int   check(...);
public:
  static constexpr bool value = (sizeof(check<T>(nullptr)) == sizeof(char));
};

template <typename T>
inline constexpr bool is_class_v = is_class<T>::value;

/**
 * @brief Checks whether Base is a base class of Derived.
 *
 * Requires both types to be class types to avoid the false-positive case
 * where void* accepts any pointer (e.g. is_base_of<void, int> was true).
 */
template <typename Base, typename Derived> struct is_base_of {
private:
  static constexpr char test(const Base *);
  static constexpr int  test(...);

public:
  static constexpr bool value =
      is_class<Base>::value &&
      is_class<Derived>::value &&
      sizeof(test(static_cast<Derived *>(nullptr))) == sizeof(char);
};

/**
 * @brief Alias to directly obtain the boolean value for is_base_of.
 */
template <typename Base, typename Derived>
inline constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

template <typename T> struct remove_pointer      { using type = T; };
template <typename T> struct remove_pointer<T *>  { using type = T; };
template <typename T> struct remove_pointer<T * const> { using type = T; };
template <typename T> using remove_pointer_t = typename remove_pointer<T>::type;

template <typename T> struct is_pointer { static constexpr bool value = false; };
template <typename T> struct is_pointer<T *> { static constexpr bool value = true; };
template <typename T> inline constexpr bool is_pointer_v = is_pointer<T>::value;

template <typename T> struct is_floating_point { static constexpr bool value = false; };
template <> struct is_floating_point<float>       { static constexpr bool value = true; };
template <> struct is_floating_point<double>      { static constexpr bool value = true; };
template <> struct is_floating_point<long double> { static constexpr bool value = true; };
template <typename T> inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

template <typename T> struct is_signed { static constexpr bool value = false; };
template <> struct is_signed<signed char>  { static constexpr bool value = true; };
template <> struct is_signed<short>        { static constexpr bool value = true; };
template <> struct is_signed<int>          { static constexpr bool value = true; };
template <> struct is_signed<long>         { static constexpr bool value = true; };
template <> struct is_signed<long long>    { static constexpr bool value = true; };
template <> struct is_signed<float>        { static constexpr bool value = true; };
template <> struct is_signed<double>       { static constexpr bool value = true; };
template <> struct is_signed<long double>  { static constexpr bool value = true; };
template <typename T> inline constexpr bool is_signed_v = is_signed<T>::value;

template <bool B, typename T, typename F> struct conditional      { using type = T; };
template <typename T, typename F> struct conditional<false, T, F> { using type = F; };
template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// Trivially-constructible/destructible/copyable — wrap compiler builtins so
// callers don't scatter __is_trivially_* throughout the codebase.
template <typename T> struct is_trivially_constructible {
  static constexpr bool value = __is_trivially_constructible(T);
};
template <typename T>
inline constexpr bool is_trivially_constructible_v = is_trivially_constructible<T>::value;

template <typename T> struct is_trivially_destructible {
  static constexpr bool value = __is_trivially_destructible(T);
};
template <typename T>
inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;

template <typename T> struct is_trivially_copyable {
  static constexpr bool value = __is_trivially_copyable(T);
};
template <typename T>
inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;

// void_t: maps any well-formed types to void — detection idiom helper
template <typename...> using void_t = void;

// declval: produces an rvalue reference to T for use in unevaluated contexts
template <typename T> T &&declval() noexcept;

} // namespace traits
} // namespace fk
