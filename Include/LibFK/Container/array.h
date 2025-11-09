#pragma once

#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

/**
 * @brief Fixed-size array container (similar to std::array).
 *
 * @tparam T Element type.
 * @tparam N Number of elements.
 */
template <typename T, size_t N> struct array {
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = T *;
  using const_iterator = const T *;

  T data_[N]; ///< Raw storage.

  /// @return Number of elements in the array.
  [[nodiscard]] constexpr size_type size() const noexcept { return N; }

  /// @return True if the array is empty (always false if N > 0).
  [[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }

  /// @return Reference to element at index i (no bounds checking).
  constexpr reference operator[](size_type i) noexcept { return data_[i]; }

  /// @return Const reference to element at index i (no bounds checking).
  constexpr const_reference operator[](size_type i) const noexcept {
    return data_[i];
  }

  /// @return Pointer to first element.
  constexpr pointer data() noexcept { return data_; }

  /// @return Const pointer to first element.
  constexpr const_pointer data() const noexcept { return data_; }

  /// @return Iterator to first element.
  constexpr iterator begin() noexcept { return data_; }

  /// @return Iterator to one-past-the-last element.
  constexpr iterator end() noexcept { return data_ + N; }

  /// @return Const iterator to first element.
  constexpr const_iterator begin() const noexcept { return data_; }

  /// @return Const iterator to one-past-the-last element.
  constexpr const_iterator end() const noexcept { return data_ + N; }

  /// @return Const iterator to first element.
  constexpr const_iterator cbegin() const noexcept { return data_; }

  /// @return Const iterator to one-past-the-last element.
  constexpr const_iterator cend() const noexcept { return data_ + N; }

  /// @return Reference to first element.
  constexpr reference front() noexcept { return data_[0]; }

  /// @return Const reference to first element.
  constexpr const_reference front() const noexcept { return data_[0]; }

  /// @return Reference to last element.
  constexpr reference back() noexcept { return data_[N - 1]; }

  /// @return Const reference to last element.
  constexpr const_reference back() const noexcept { return data_[N - 1]; }

  /// @brief Fill the entire array with a value.
  constexpr void fill(const T &value) noexcept {
    for (size_type i = 0; i < N; ++i) {
      data_[i] = value;
    }
  }
};
