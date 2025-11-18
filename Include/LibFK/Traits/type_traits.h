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
template <typename T>
inline constexpr bool is_array_v = is_array<T>::value;

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
template <typename T>
using remove_extent_t = typename remove_extent<T>::type;

} // namespace traits
} // namespace fk