#pragma once

#include <LibC/stddef.h>
#include <LibFK/Traits/type_traits.h>

/*
 * @brief Fixed-capacity vector with stack allocation.
 *
 * Provides a simple vector-like container with a compile-time
 * fixed capacity. Does not allocate memory dynamically.
 *
 * @tparam T Type of elements
 * @tparam N Maximum number of elements
 */
template <typename T, size_t N> struct static_vector {
  T data[N];        ///< Internal storage
  size_t count = 0; ///< Current number of elements

  /**
   * @brief Get the current number of elements in the vector.
   * @return Number of elements
   */
  constexpr size_t size() const { return count; }

  /**
   * @brief Get the maximum capacity of the vector.
   * @return Maximum number of elements
   */
  constexpr size_t capacity() const { return N; }

  /**
   * @brief Add a new element at the end of the vector.
   * @param value Element to add
   * @return True if the element was added, false if vector is full
   */
  bool push_back(const T &value) {
    if (count >= N)
      return false;
    data[count++] = value;
    return true;
  }

  /**
   * @brief Access element by index.
   * @param i Index of the element
   * @return Reference to element at index i
   */
  T &operator[](size_t i) { return data[i]; }

  /**
   * @brief Const access to element by index.
   * @param i Index of the element
   * @return Const reference to element at index i
   */
  const T &operator[](size_t i) const { return data[i]; }

  /**
   * @brief Get pointer to first element (for range-based for loops).
   * @return Pointer to first element
   */
  T *begin() { return data; }

  /**
   * @brief Get pointer to one past last element (for range-based for loops).
   * @return Pointer to one past last element
   */
  T *end() { return data + count; }

  /**
   * @brief Const version of begin().
   * @return Const pointer to first element
   */
  const T *begin() const { return data; }

  /**
   * @brief Const version of end().
   * @return Const pointer to one past last element
   */
  const T *end() const { return data + count; }
};
