#include <LibC/assert.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Types/types.h> // For fk::types::move

namespace fk {
namespace text {

fk::core::Result<void, fk::core::Error> String::_initialize_buffer(size_t initial_cap) {
  auto res = ensure_capacity(initial_cap);
  if (res.is_error()) {
    fk::algorithms::kerror("String", "Failed to allocate initial buffer.");
    m_metadata = {0, 0};
    return res.error();
  }
  m_data.ptr()[0] = '\0';
  return fk::core::Result<void>();
}

String::String()
    : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair with 0 length and 0 capacity
  _initialize_buffer(16);
}

fk::core::Result<void, fk::core::Error> String::_copy_from_cstring(const char* s, size_t len) {
  if (!s) {
    m_metadata.first = 0;
    if (m_metadata.second > 0) {
      m_data.ptr()[0] = '\0';
    } else {
      return _initialize_buffer(1);
    }
    return fk::core::Result<void>();
  }
  m_metadata.first = len;
  auto res = ensure_capacity(m_metadata.first + 1);
  if (res.is_error()) {
    fk::algorithms::kerror("String", "Failed to allocate memory for string copy from cstring.");
    m_data = nullptr;
    m_metadata = {0, 0};
    return res.error();
  }
  memcpy(m_data.ptr(), s, m_metadata.first);
  m_data.ptr()[m_metadata.first] = '\0';
  return fk::core::Result<void>();
}

String::String(const char *s)
    : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair
  _copy_from_cstring(s, strlen(s));
}

String::String(const String &other)
    : m_data(nullptr),
      m_metadata({0, 0}) { // Initialize metadata pair (length set by copy_from_string)
  _copy_from_string(other);
}

fk::core::Result<void, fk::core::Error> String::_copy_from_string(const String& other) {
  m_metadata.first = other.m_metadata.first;
  auto res = ensure_capacity(m_metadata.first + 1);
  if (res.is_error()) {
    fk::algorithms::kerror("String", "Failed to allocate memory for string copy from String.");
    m_data = nullptr;
    m_metadata = {0, 0};
    return res.error();
  }
  memcpy(m_data.ptr(), other.m_data.ptr(), m_metadata.first);
  m_data.ptr()[m_metadata.first] = '\0';
  return fk::core::Result<void>();
}

String::String(String &&other) noexcept
    : m_data(fk::types::move(other.m_data)),
      m_metadata(other.m_metadata) { // Move metadata pair
  // OwnPtr's move constructor handles nulling out the source.
  other.m_metadata = {0, 0}; // Reset other's metadata
}

String::~String() {
  // OwnPtr handles deletion, no need for manual delete[] m_data;
}

String &String::operator=(const String &other) {
  if (this == &other) {
    return *this;
  }
  _copy_from_string(other);
  return *this;
}

String &String::operator=(String &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  m_data = fk::types::move(other.m_data); // Move OwnPtr
  m_metadata = other.m_metadata;          // Move metadata pair
  // OwnPtr's move assignment handles nulling out the source.
  other.m_metadata = {0, 0}; // Reset other's metadata
  return *this;
}

String &String::operator=(const char *s) {
  _copy_from_cstring(s, strlen(s));
  return *this;
}

void String::clear() {
  m_metadata.first = 0; // Reset length
  if (m_data) {
    m_data.ptr()[0] = '\0'; // Use ptr()
  }
}

fk::core::Result<void, fk::core::Error> String::reserve(size_t new_cap) {
  if (new_cap > m_metadata.second) { // Use m_metadata.second for capacity
    return ensure_capacity(new_cap);
  }
  return fk::core::Result<void>(); // Already has enough capacity, success
}

fk::core::Result<void, fk::core::Error> String::_append_data(const char* data, size_t len) {
  if (!data || len == 0) {
    return fk::core::Result<void>();
  }
  auto res = ensure_capacity(m_metadata.first + len + 1);
  if (res.is_error()) {
    fk::algorithms::kerror("String", "Failed to allocate memory during append data.");
    return res.error();
  }
  memcpy(m_data.ptr() + m_metadata.first, data, len);
  m_metadata.first += len;
  m_data.ptr()[m_metadata.first] = '\0';
  return fk::core::Result<void>();
}

String &String::append(const char *s) {
  _append_data(s, strlen(s));
  return *this;
}

String &String::append(const String &str) {
  _append_data(str.m_data.ptr(), str.m_metadata.first);
  return *this;
}

void String::push_back(char c) {
  auto res = ensure_capacity(m_metadata.first + 2);
  if (res.is_error()) {
    fk::algorithms::kerror("String",
                           "Failed to allocate memory during push_back.");
    // String remains in its current state, but push_back failed.
    return;
  }
  // If ensure_capacity succeeded, m_data is now valid.
  m_data.ptr()[m_metadata.first++] = c;  // Use ptr()
  m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
}

char &String::operator[](size_t index) {
  ASSERT(index < m_metadata.first); // Use m_metadata.first for length
  return m_data.ptr()[index];       // Use ptr()
}

const char &String::operator[](size_t index) const {
  ASSERT(index < m_metadata.first); // Use m_metadata.first for length
  return m_data.ptr()[index];       // Use ptr()
}

fk::core::Result<void, fk::core::Error>
String::ensure_capacity(size_t min_cap) {
  if (m_metadata.second >= min_cap) { // Use m_metadata.second for capacity
    return fk::core::Result<void>();  // Already has enough capacity, success
  }
  size_t new_cap = m_metadata.second == 0 ? 16 : m_metadata.second * 2;
  if (new_cap < min_cap) {
    new_cap = min_cap;
  }
  char *new_data = new char[new_cap];
  if (!new_data) { // Check for allocation failure
    // Log error and return OutOfMemory
    fk::algorithms::kerror("String",
                           "Failed to allocate memory in ensure_capacity.");
    return fk::core::Error::OutOfMemory;
  }
  if (m_data) {
    memcpy(new_data, m_data.ptr(),
           m_metadata.first +
               1); // Copy existing data including null terminator, use ptr()
    delete[] m_data.ptr(); // Use ptr() for delete[]
  }
  m_data = new_data; // OwnPtr assignment should handle ownership transfer
  m_metadata.second = new_cap;     // Update capacity
  return fk::core::Result<void>(); // Success
}

} // namespace text
} // namespace fk
