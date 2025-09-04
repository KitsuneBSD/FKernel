#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>

template <typename T> class optional {
private:
  bool has_value_;
  alignas(T) unsigned char storage[sizeof(T)];

public:
  constexpr optional() : has_value_(false) {}

  optional(const T &value) : has_value_(true) {
    memcpy(storage, &value, sizeof(T));
  }

  optional(const optional &other) : has_value_(other.has_value_) {
    if (has_value_) {
      memcpy(storage, other.storage, sizeof(T));
    }
  }

  ~optional() { reset(); }

  optional &operator=(const optional &other) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        memcpy(storage, other.storage, sizeof(T));
        has_value_ = true;
      }
    }
    return *this;
  }

  constexpr bool has_value() const { return has_value_; }

  T &value() { return *reinterpret_cast<T *>(storage); }

  const T &value() const { return *reinterpret_cast<const T *>(storage); }

  void reset() {
    if (has_value_) {
      memset(storage, 0, sizeof(T));
      has_value_ = false;
    }
  }
};
