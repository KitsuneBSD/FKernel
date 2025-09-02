#pragma once

#include <LibC/stddef.h>

template <typename T> struct span {
  T *ptr = nullptr;
  size_t len = 0;

  constexpr span() = default;
  constexpr span(T *p, size_t n) : ptr(p), len(n) {}

  T &operator[](size_t i) { return ptr[i]; }
  const T &operator[](size_t i) const { return ptr[i]; }

  constexpr size_t size() const { return len; }
  T *begin() { return ptr; }
  T *end() { return ptr + len; }
  const T *begin() const { return ptr; }
  const T *end() const { return ptr + len; }
};
