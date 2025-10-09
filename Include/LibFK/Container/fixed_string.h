#pragma once

#include <LibC/stddef.h>

/**
 * @brief Fixed-capacity string container with null-termination.
 *
 * Provides safe, stack-allocated string handling without dynamic memory.
 * Similar to std::string but with a fixed buffer size known at compile-time.
 *
 * @tparam N Maximum number of characters (excluding null terminator).
 */
template <size_t N>
struct fixed_string
{
  char buffer[N + 1] = {}; ///< Internal buffer (always null-terminated).
  size_t length = 0;       ///< Current string length (not counting terminator).

  fixed_string(const char *s)
  {
    size_t i = 0;
    while (s[i] != '\0' && i < N)
    {
      buffer[i] = s[i];
      ++i;
    }
    length = i;
    buffer[i] = '\0';
  }

  fixed_string() : buffer{}, length(0) { buffer[0] = '\0'; }

  /// @return Current length of the string.
  [[nodiscard]] constexpr size_t size() const noexcept { return length; }

  /// @return Maximum number of characters that can be stored (excluding
  /// terminator).
  [[nodiscard]] constexpr size_t capacity() const noexcept { return N; }

  /// @return True if the string is empty.
  [[nodiscard]] constexpr bool empty() const noexcept { return length == 0; }

  /// @brief Clears the string (sets length to zero).
  constexpr void clear() noexcept
  {
    length = 0;
    buffer[0] = '\0';
  }

  /**
   * @brief Append a C-string.
   * @param s Null-terminated string to append.
   * @return True if append succeeded, false if there was not enough capacity.
   */
  constexpr bool append(const char *s) noexcept
  {
    size_t i = 0;
    while (s[i] != '\0')
    {
      if (length >= N)
        return false;
      buffer[length++] = s[i++];
    }
    buffer[length] = '\0';
    return true;
  }

  /**
   * @brief Append a single character.
   * @param c Character to append.
   * @return True if append succeeded, false if buffer is full.
   */
  constexpr bool push_back(char c) noexcept
  {
    if (length >= N)
      return false;
    buffer[length++] = c;
    buffer[length] = '\0';
    return true;
  }

  /// @return Reference to character at index (unchecked).
  constexpr char &operator[](size_t i) noexcept { return buffer[i]; }

  /// @return Const reference to character at index (unchecked).
  constexpr const char &operator[](size_t i) const noexcept
  {
    return buffer[i];
  }

  /// @return Pointer to null-terminated string (mutable).
  constexpr char *c_str() noexcept { return buffer; }

  /// @return Pointer to null-terminated string (const).
  constexpr const char *c_str() const noexcept { return buffer; }
};
