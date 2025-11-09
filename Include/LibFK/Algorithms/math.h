#pragma once

#include <LibFK/Traits/type_traits.h>

template <typename T> T floor(T x, T y) {
  static_assert(is_integral<T>::value, "T must be a integral value");

  T q = x / y;

  if ((x % y != 0) && ((x < 0) != (y < 0)))
    q -= 1;

  return q;
}
