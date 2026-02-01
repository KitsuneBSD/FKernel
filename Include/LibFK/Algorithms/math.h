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

} // namespace algorithms
} // namespace fk