#pragma once

#include <LibC/stddef.h>
#include <LibFK/Traits/type_traits.h>
/**
 * @brief Base class for fixed-size or fixed-capacity containers.
 * Provides a common interface for size, capacity, empty/full checks, and clear.
 * 
 * @tparam Derived The derived container class implementing specific behavior.
 * @tparam T The type of elements stored in the container.
 */
template <typename Derived, typename T>
class ContainerBase {
public:
  using value_type = T;
  using size_type = size_t;

  constexpr ContainerBase() noexcept = default;

  /**
   * @brief Get the number of elements currently stored in the container.
   * @return size_type The current size.
   */
  size_type size() const noexcept {
    return static_cast<const Derived *>(this)->size();
  }

  /**
   * @brief Get the maximum capacity of the container.
   * @return size_type The maximum capacity.
   */
  size_type capacity() const noexcept {
    return static_cast<const Derived *>(this)->capacity();
  }

  /**
   * @brief Check if the container is empty.
   * @return true If the container is empty.
   * @return false Otherwise.
   */
  bool is_empty() const noexcept { return size() == 0; }

  /**
   * @brief Check if the container is full.
   * @return true If the container is full.
   * @return false Otherwise.
   */
  bool is_full() const noexcept { return size() >= capacity(); }

  /**
   * @brief Clear the contents of the container.
   */
  void clear() noexcept { static_cast<Derived *>(this)->clear(); }
};
