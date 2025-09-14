#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/new.h>

template <typename T> class optional {
private:
  bool has_value_{false};
  alignas(T) unsigned char storage[sizeof(T)];

  T *ptr() { return reinterpret_cast<T *>(storage); }
  const T *ptr() const { return reinterpret_cast<const T *>(storage); }

public:
  constexpr optional() = default;

  optional(const T &value) : has_value_(true) { new (storage) T(value); }

  optional(const optional &other) : has_value_(other.has_value_) {
    if (has_value_)
      new (storage) T(*other.ptr());
  }

  ~optional() { reset(); }

  optional &operator=(const optional &other) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        new (storage) T(*other.ptr());
        has_value_ = true;
      }
    }
    return *this;
  }

  constexpr bool has_value() const { return has_value_; }

  T &value() { return *ptr(); }
  const T &value() const { return *ptr(); }

  void reset() {
    if (has_value_) {
      ptr()->~T();
      has_value_ = false;
    }
  }
};
