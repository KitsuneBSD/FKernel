#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace utilities {

/**
 * @brief A simple pair of two values, similar to std::pair.
 *
 * Stores two related objects of possibly different types.
 *
 * @tparam T1 Type of the first element
 * @tparam T2 Type of the second element
 */
template <typename T1, typename T2> struct Pair {
  T1 first;  ///< First element
  T2 second; ///< Second element

  /**
   * @brief Default constructor. Initializes both elements with default
   * constructors.
   */
  constexpr Pair() : first(), second() {}

  /**
   * @brief Construct a Pair with given values.
   * @param a Value for the first element
   * @param b Value for the second element
   */
  constexpr Pair(const T1 &a, const T2 &b) : first(a), second(b) {}

  constexpr Pair(T1 &&a, T2 &&b)
      : first(fk::types::move(a)), second(fk::types::move(b)) {}

  constexpr Pair(const Pair &other)
      : first(other.first), second(other.second) {}

  constexpr Pair(Pair &&other)
      : first(fk::types::move(other.first)), second(fk::types::move(other.second)) {}

  Pair &operator=(const Pair &other) {
    if (this != &other) {
      first = other.first;
      second = other.second;
    }
    return *this;
  }

  Pair &operator=(Pair &&other) {
    if (this != &other) {
      first = fk::types::move(other.first);
      second = fk::types::move(other.second);
    }
    return *this;
  }

  /**
   * @brief Equality operator.
   * @param other Pair to compare
   * @return true if both elements are equal
   */
  constexpr bool operator==(const Pair &other) const {
    return first == other.first && second == other.second;
  }

  /**
   * @brief Inequality operator.
   * @param other Pair to compare
   * @return true if either element differs
   */
  constexpr bool operator!=(const Pair &other) const {
    return !(*this == other);
  }
};

} // namespace utilities
} // namespace fk