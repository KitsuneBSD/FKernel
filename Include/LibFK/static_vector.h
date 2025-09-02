#pragma once

#include <LibC/stddef.h>
#include <LibFK/type_traits.h>

template <typename T, size_t N> struct static_vector {
  T data[N];
  size_t count = 0;

  constexpr size_t size() const { return count; }
  constexpr size_t capacity() const { return N; }

  bool push_back(const T &value) {
    if (count >= N)
      return false;
    data[count++] = value;
    return true;
  }

  T &operator[](size_t i) { return data[i]; }
  const T &operator[](size_t i) const { return data[i]; }

  T *begin() { return data; }
  T *end() { return data + count; }
  const T *begin() const { return data; }
  const T *end() const { return data + count; }
};
