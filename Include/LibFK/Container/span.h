#pragma once

#include <LibC/stddef.h>
#include <LibFK/Base/container_base.h>
/**
 * @brief Non-owning view over a contiguous sequence of objects.
 *
 * Similar to std::span, this class provides a lightweight wrapper over a
 * pointer and a length, without owning the underlying memory.
 *
 * @tparam T Type of the elements in the span
 */
template <typename T> struct span : ContainerBase<span<T>, T> {
  T *ptr = nullptr; ///< Pointer to the first element
  size_t len = 0;   ///< Number of elements

  /**
   * @brief Default constructor. Creates an empty span.
   */
  constexpr span() = default;

  /**
   * @brief Construct a span from pointer and length.
   * @param p Pointer to the first element
   * @param n Number of elements
   */
  constexpr span(T *p, size_t n) : ptr(p), len(n) {}

  /**
   * @brief Access element at index.
   * @param i Index of the element
   * @return Reference to element at index i
   */
  T &operator[](size_t i) { return ptr[i]; }

  /**
   * @brief Access element at index (const version).
   * @param i Index of the element
   * @return Const reference to element at index i
   */
  const T &operator[](size_t i) const { return ptr[i]; }

  /**
   * @brief Get the number of elements in the span.
   * @return Number of elements
   */
  constexpr size_t size() const { return len; }

  /**
   * @brief Get pointer to the first element (for range-based for loops).
   * @return Pointer to the first element
   */
  T *begin() { return ptr; }

  /**
   * @brief Get pointer to one past the last element (for range-based for
   * loops).
   * @return Pointer to one past the last element
   */
  T *end() { return ptr + len; }

  /**
   * @brief Const version of begin().
   * @return Const pointer to the first element
   */
  const T *begin() const { return ptr; }

  /**
   * @brief Const version of end().
   * @return Const pointer to one past the last element
   */
  const T *end() const { return ptr + len; }
};
