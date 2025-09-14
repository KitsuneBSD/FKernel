#pragma once

// Enable_if
// When we dont define ::type
template <bool B, typename T = void> struct enable_if {};

template <typename T> struct enable_if<true, T> {
  using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// Is_Same
template <typename T, typename U> struct is_same {
  static constexpr bool value = false;
};

template <typename T> struct is_same<T, T> {
  static constexpr bool value = true;
};

template <typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;

// Remove Const
template <typename T> struct remove_const {
  using type = T;
};

template <typename T> struct remove_const<const T> {
  using type = T;
};

template <typename T> using remove_const_t = typename remove_const<T>::type;

// Remove Reference

template <typename T> struct remove_reference {
  using type = T;
};

template <typename T> struct remove_reference<T &> {
  using type = T;
};

template <typename T> struct remove_reference<T &&> {
  using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

// Is_Integral

template <typename T> struct is_integral {
  static constexpr bool value = false;
};

#define FK_DEFINE_INTEGRAL(T)                                                  \
  template <> struct is_integral<T> {                                          \
    static constexpr bool value = true;                                        \
  };

FK_DEFINE_INTEGRAL(bool)
FK_DEFINE_INTEGRAL(char)
FK_DEFINE_INTEGRAL(signed char)
FK_DEFINE_INTEGRAL(unsigned char)
FK_DEFINE_INTEGRAL(short)
FK_DEFINE_INTEGRAL(unsigned short)
FK_DEFINE_INTEGRAL(int)
FK_DEFINE_INTEGRAL(unsigned int)
FK_DEFINE_INTEGRAL(long)
FK_DEFINE_INTEGRAL(unsigned long)
FK_DEFINE_INTEGRAL(long long)
FK_DEFINE_INTEGRAL(unsigned long long)

#undef FK_DEFINE_INTEGRAL

template <typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;
