#pragma once

#include <LibFK/Traits/type_traits.h>

namespace fk {
namespace algorithms {

template <typename T>
constexpr const T& min(const T& a, const T& b) {
    return (b < a) ? b : a;
}

template <typename T>
constexpr const T& max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

template <typename T> T floor(T x, T y) {
  static_assert(fk::traits::is_integral<T>::value, "T must be a integral value");

  T q = x / y;

  if ((x % y != 0) && ((x < 0) != (y < 0)))
    q -= 1;

  return q;
}

template <typename T>
constexpr T abs(T x) { return x < 0 ? -x : x; }

template <typename T>
constexpr void swap(T& a, T& b) {
  T tmp = static_cast<T&&>(a);
  a = static_cast<T&&>(b);
  b = static_cast<T&&>(tmp);
}

template <typename T>
constexpr T clamp(T value, T lo, T hi) {
  return value < lo ? lo : value > hi ? hi : value;
}

} // namespace algorithms
} // namespace fk