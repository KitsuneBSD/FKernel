#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>       // For strcmp, strlen
#include <LibFK/Core/Result.h> // Include Result for return types
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Utilities/pair.h> // Include Pair definition

namespace fk {
namespace text { // As per GEMINI.md

class String {
public:
  using iterator = char *;
  using const_iterator = const char *;

  String();
  String(const char *s);
  String(const String &other);
  String(String &&other) noexcept;

  ~String();

  String &operator=(const String &other);
  String &operator=(String &&other) noexcept;
  String &operator=(const char *s);

  // Access length and capacity from the metadata pair
  size_t length() const { return m_metadata.first; }
  size_t size() const { return m_metadata.first; } // Alias for length
  size_t capacity() const { return m_metadata.second; }

  bool is_empty() const { return length() == 0; }
  const char *c_str() const { return m_data.ptr() ? m_data.ptr() : ""; }
  char *data() { return m_data.ptr(); }
  const char *data() const { return m_data.ptr(); }

  void clear();
  // Changed return type to Result
  fk::core::Result<void, fk::core::Error> reserve(size_t new_cap);

  String &append(const char *s);
  String &append(const String &str);
  void push_back(char c);

  String &operator+=(const char *s) { return append(s); }
  String &operator+=(const String &str) { return append(str); }

  char &operator[](size_t index);
  const char &operator[](size_t index) const;

  iterator begin() { return m_data.ptr(); }
  iterator end() { return m_data.ptr() + length(); }
  const_iterator begin() const { return m_data.ptr(); }
  const_iterator end() const { return m_data.ptr() + length(); }
  const_iterator cbegin() const { return m_data.ptr(); }
  const_iterator cend() const { return m_data.ptr() + length(); }

private:
  // Changed return type to Result
  fk::core::Result<void, fk::core::Error> ensure_capacity(size_t min_cap);
  fk::core::Result<void, fk::core::Error> _initialize_buffer(size_t initial_cap);
  fk::core::Result<void, fk::core::Error> _copy_from_cstring(const char* s, size_t len);
  fk::core::Result<void, fk::core::Error> _copy_from_string(const String& other);
  fk::core::Result<void, fk::core::Error> _append_data(const char* data, size_t len);

  // Changed m_data to OwnPtr for automatic memory management
  fk::memory::OwnPtr<char[]> m_data;
  // Group length and capacity into a Pair to satisfy the two-instance-variable
  // rule.
  fk::utilities::Pair<size_t, size_t>
      m_metadata; // first: length, second: capacity
};

inline String operator+(const String &lhs, const String &rhs) {
  String result;
  // Note: reserve now returns a Result, so this needs to be handled if we want
  // robust error propagation. For now, assuming reserve succeeds or
  // panics/logs.
  result.reserve(lhs.length() + rhs.length() + 1);
  result.append(lhs);
  result.append(rhs);
  return result;
}

inline String operator+(const String &lhs, const char *rhs) {
  if (!rhs)
    return lhs;
  String result;
  result.reserve(lhs.length() + strlen(rhs) + 1);
  result.append(lhs);
  result.append(rhs);
  return result;
}

inline String operator+(const char *lhs, const String &rhs) {
  if (!lhs)
    return rhs;
  String result;
  result.reserve(strlen(lhs) + rhs.length() + 1);
  result.append(lhs);
  result.append(rhs);
  return result;
}

inline bool operator==(const String &lhs, const String &rhs) {
  return lhs.length() == rhs.length() && strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

inline bool operator==(const String &lhs, const char *rhs) {
  if (!rhs)
    return lhs.is_empty();
  return strcmp(lhs.c_str(), rhs) == 0;
}

inline bool operator==(const char *lhs, const String &rhs) {
  if (!lhs)
    return rhs.is_empty();
  return strcmp(lhs, rhs.c_str()) == 0;
}

inline bool operator!=(const String &lhs, const String &rhs) {
  return !(lhs == rhs);
}

inline bool operator!=(const String &lhs, const char *rhs) {
  return !(lhs == rhs);
}

inline bool operator!=(const char *lhs, const String &rhs) {
  return !(lhs == rhs);
}

} // namespace text
} // namespace fk
