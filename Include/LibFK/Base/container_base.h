#pragma once

#include <LibC/stddef.h>

/**
 * @brief Base class for fixed-size or fixed-capacity containers.
 * Provides common interface for size, capacity, empty/full checks, and clear.
 */
template <typename Derived, typename T> class ContainerBase {
public:
  using value_type = T;
  using size_type = size_t;

  constexpr ContainerBase() = default;

  // Number of elements currently stored
  size_type size() const { return static_cast<const Derived *>(this)->size(); }

  // Maximum capacity
  size_type capacity() const {
    return static_cast<const Derived *>(this)->capacity();
  }

  // Empty?
  bool is_empty() const { return size() == 0; }

  // Full?
  bool is_full() const { return size() >= capacity(); }

  // Clear contents
  void clear() { static_cast<Derived *>(this)->clear(); }
};
