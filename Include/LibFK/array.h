#pragma once

#include <LibC/stddef.h>
#include <LibFK/type_traits.h>

template <typename T, size_t N> struct array {
  T array[N];

  constexpr size_t size() const { return N; }
  constexpr T &operator[](size_t i) { return array[i]; }
  constexpr const T &operator[](size_t i) const { return array[i]; }

  T *begin() { return array; }
  T *end() { return array + N; }

  const T *begin() const { return array; }
  const T *end() const { return array + N; }
};
