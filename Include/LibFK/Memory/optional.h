#pragma once

#include <LibC/stddef.h>
#include <LibFK/Memory/Allocators/new.h>

namespace fk {
namespace memory {

/**
 * @brief Minimal freestanding implementation of std::optional.
 *
 * Stores an object of type T in-place without dynamic allocation.
 *
 * @tparam T Type of the stored value
 */
template <typename T> class optional {
private:
  bool has_value_{false};
  alignas(T) unsigned char storage[sizeof(T)];

  T *ptr() { return reinterpret_cast<T *>(storage); }
  const T *ptr() const { return reinterpret_cast<const T *>(storage); }

public:
  constexpr optional() = default;

  optional(T &&value) : has_value_(true) {
    new (storage) T(static_cast<T &&>(value));
  }

  optional(const T &val) : has_value_(true) {
    new (storage) T(val);
  }

  optional(const optional &other) : has_value_(other.has_value_) {
    if (has_value_)
      new (storage) T(*other.ptr());
  }

  optional(optional &&other) : has_value_(other.has_value_) {
    if (has_value_) {
      new (storage) T(static_cast<T &&>(*other.ptr()));
      other.reset();
    }
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

  optional &operator=(optional &&other) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        new (storage) T(static_cast<T &&>(*other.ptr()));
        has_value_ = true;
        other.reset();
      }
    }
    return *this;
  }

  constexpr bool has_value() const { return has_value_; }
  explicit constexpr operator bool() const { return has_value_; }

  T &value() {
    if (!has_value_) __builtin_trap();
    return *ptr();
  }

  const T &value() const {
    if (!has_value_) __builtin_trap();
    return *ptr();
  }

  T &operator*() { return *ptr(); }
  const T &operator*() const { return *ptr(); }

  T *operator->() { return ptr(); }
  const T *operator->() const { return ptr(); }

  template <typename U>
  T value_or(U &&default_value) const {
    if (has_value_) return *ptr();
    return static_cast<T>(static_cast<U &&>(default_value));
  }

  template <typename... Args>
  T &emplace(Args &&...args) {
    reset();
    new (storage) T(static_cast<Args &&>(args)...);
    has_value_ = true;
    return *ptr();
  }

  void reset() {
    if (has_value_) {
      ptr()->~T();
      has_value_ = false;
    }
  }
};

} // namespace memory
} // namespace fk
