#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>       // For strcmp, strlen
#include <LibFK/Core/result.h>
#include <LibFK/Utilities/pair.h>

namespace fk {
namespace text { // As per GEMINI.md

class String {
public:
  using iterator = char *;
  using const_iterator = const char *;

  static constexpr size_t NPOS = ~size_t(0);

  String();
  String(const char *s);
  String(const String &other);
  String(String &&other) noexcept;

  ~String();

  String &operator=(const String &other);
  String &operator=(String &&other) noexcept;
  String &operator=(const char *s);

  size_t length() const { return m_metadata.first; }
  size_t size() const { return m_metadata.first; }
  size_t capacity() const { return m_is_sso() ? SSO_CAP : m_metadata.second; }

  bool is_empty() const { return length() == 0; }
  const char *c_str() const { return m_data_ptr(); }
  char *data() { return m_data_ptr(); }
  const char *data() const { return m_data_ptr(); }

  void clear();
  fk::core::Result<void, fk::core::Error> reserve(size_t new_cap);

  String &append(const char *s);
  String &append(const String &str);
  void push_back(char c);

  String &operator+=(const char *s) { return append(s); }
  String &operator+=(const String &str) { return append(str); }

  char &operator[](size_t index);
  const char &operator[](size_t index) const;

  iterator begin() { return m_data_ptr(); }
  iterator end() { return m_data_ptr() + length(); }
  const_iterator begin() const { return m_data_ptr(); }
  const_iterator end() const { return m_data_ptr() + length(); }
  const_iterator cbegin() const { return m_data_ptr(); }
  const_iterator cend() const { return m_data_ptr() + length(); }

  String substr(size_t pos, size_t count = NPOS) const;
  size_t find(char c, size_t pos = 0) const;
  size_t find(const char *s, size_t pos = 0) const;
  size_t find(const String &s, size_t pos = 0) const;
  size_t rfind(char c, size_t pos = NPOS) const;
  size_t rfind(const char *s, size_t pos = NPOS) const;

  bool starts_with(const char *s) const;
  bool starts_with(const String &s) const;
  bool ends_with(const char *s) const;
  bool ends_with(const String &s) const;
  bool contains(const char *s) const;
  bool contains(const String &s) const;

  String &trim();
  String to_upper() const;
  String to_lower() const;

  String &erase(size_t pos, size_t count = NPOS);
  String &insert(size_t pos, const char *s);
  String &replace(size_t pos, size_t count, const char *s);

private:
  static constexpr size_t SSO_CAP = 15;

  fk::core::Result<void, fk::core::Error> ensure_capacity(size_t min_cap);
  fk::core::Result<void, fk::core::Error> _copy_from_cstring(const char* s, size_t len);
  fk::core::Result<void, fk::core::Error> _copy_from_string(const String& other);
  fk::core::Result<void, fk::core::Error> _append_data(const char* data, size_t len);

  // m_metadata.second == 0  →  SSO mode (data stored inline in m_data.sso)
  // m_metadata.second  > 0  →  heap mode (data at m_data.heap_ptr)
  union DataStorage {
    char* heap_ptr;
    char  sso[SSO_CAP + 1];
    DataStorage() : heap_ptr(nullptr) {}
  } m_data;
  fk::utilities::Pair<size_t, size_t> m_metadata; // {length, capacity}

  bool  m_is_sso() const { return m_metadata.second == 0; }
  char* m_data_ptr() const {
    return m_is_sso() ? const_cast<char*>(m_data.sso) : m_data.heap_ptr;
  }
};

inline String operator+(const String &lhs, const String &rhs) {
  String result;
  if (result.reserve(lhs.length() + rhs.length() + 1).is_error())
    return result;
  result.append(lhs);
  result.append(rhs);
  return result;
}

inline String operator+(const String &lhs, const char *rhs) {
  if (!rhs)
    return lhs;
  String result;
  if (result.reserve(lhs.length() + strlen(rhs) + 1).is_error())
    return result;
  result.append(lhs);
  result.append(rhs);
  return result;
}

inline String operator+(const char *lhs, const String &rhs) {
  if (!lhs)
    return rhs;
  String result;
  if (result.reserve(strlen(lhs) + rhs.length() + 1).is_error())
    return result;
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
