#pragma once

template <typename T1, typename T2> struct Pair {
  T1 first;
  T2 second;

  // Default Constructor
  constexpr Pair() : first(), second() {}
  // Constructor with Values
  constexpr Pair(const T1 &a, const T2 &b) : first(a), second(b) {}
  // Constructor with Copy
  constexpr Pair(const Pair &other)
      : first(other.first), second(other.second) {}

  Pair &operator=(const Pair &other) {
    if (this != &other) {
      first = other.first;
      second = other.second;
    }
    return *this;
  }

  constexpr bool operator==(const Pair &other) const {
    return first == other.first && second == other.second;
  }

  constexpr bool operator!=(const Pair &other) const {
    return !(*this == other);
  }
};
