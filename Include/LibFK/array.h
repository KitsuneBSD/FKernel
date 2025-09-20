#pragma once

#include <LibC/stddef.h>
#include <LibFK/type_traits.h>

template <typename T, size_t N> struct array {
  T data_[N];

  constexpr size_t size() const { return N; }
  constexpr T &operator[](size_t i) { return data_[i]; }
  constexpr const T &operator[](size_t i) const { return data_[i]; }

  T *begin() { return data_; }
  T *end() { return data_ + N; }

  constexpr void fill(const T &value) {
    for (size_t i = 0; i < N; ++i) {
      data_[i] = value;
    }
  }

  const T *begin() const { return data_; }
  const T *end() const { return data_ + N; }
};
