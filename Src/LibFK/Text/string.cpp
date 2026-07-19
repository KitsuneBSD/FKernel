#include <LibC/assert.h>
#include <LibC/ctype.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Types/types.h>

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
      m_metadata({0, 0}) {
  auto result = _copy_from_string(other);
  if (result.is_error()) {
    _initialize_buffer(1);
    fk::algorithms::kwarn("String", "Copy constructor failed, initialized empty");
  }
}

fk::core::Result<void, fk::core::Error> String::_copy_from_string(const String& other) {
  if (!other.m_data) {
    clear();
    return fk::core::Result<void>();
  }
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
  if (str.m_data && str.m_metadata.first > 0)
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
  
  // Always allocate space for null terminator
  char *new_data = new char[new_cap + 1];
  if (!new_data) {
    fk::algorithms::kerror("String",
                           "Failed to allocate memory in ensure_capacity.");
    return fk::core::Error::OutOfMemory;
  }
  
  if (m_data) {
    memcpy(new_data, m_data.ptr(), m_metadata.first);
    new_data[m_metadata.first] = '\0';
  } else {
    new_data[0] = '\0';
  }
  
  m_data = OwnPtr<char[]>(new_data); 
  m_metadata.second = new_cap;
  return fk::core::Result<void>();
}

String String::substr(size_t pos, size_t count) const {
  if (pos >= m_metadata.first)
    return String();
  size_t available = m_metadata.first - pos;
  size_t len = (count == NPOS || count > available) ? available : count;
  String result;
  if (result._append_data(m_data.ptr() + pos, len).is_error())
    return String();
  return result;
}

size_t String::find(char c, size_t pos) const {
  for (size_t i = pos; i < m_metadata.first; ++i) {
    if (m_data.ptr()[i] == c)
      return i;
  }
  return NPOS;
}

size_t String::find(const char *s, size_t pos) const {
  if (!s || !m_data)
    return NPOS;
  size_t slen = strlen(s);
  if (slen == 0)
    return pos;
  for (size_t i = pos; i + slen <= m_metadata.first; ++i) {
    if (memcmp(m_data.ptr() + i, s, slen) == 0)
      return i;
  }
  return NPOS;
}

size_t String::find(const String &s, size_t pos) const {
  return find(s.c_str(), pos);
}

size_t String::rfind(char c, size_t pos) const {
  if (m_metadata.first == 0)
    return NPOS;
  size_t i = (pos == NPOS || pos >= m_metadata.first) ? m_metadata.first - 1 : pos;
  while (true) {
    if (m_data.ptr()[i] == c)
      return i;
    if (i == 0)
      break;
    --i;
  }
  return NPOS;
}

size_t String::rfind(const char *s, size_t pos) const {
  if (!s || !m_data)
    return NPOS;
  size_t slen = strlen(s);
  if (slen == 0)
    return m_metadata.first;
  size_t start = (pos == NPOS || pos + slen > m_metadata.first)
                 ? m_metadata.first - slen
                 : pos;
  for (size_t i = start + 1; i-- > 0;) {
    if (memcmp(m_data.ptr() + i, s, slen) == 0)
      return i;
  }
  return NPOS;
}

bool String::starts_with(const char *s) const {
  if (!s)
    return true;
  size_t slen = strlen(s);
  if (slen > m_metadata.first)
    return false;
  return memcmp(c_str(), s, slen) == 0;
}

bool String::starts_with(const String &s) const {
  return starts_with(s.c_str());
}

bool String::ends_with(const char *s) const {
  if (!s)
    return true;
  size_t slen = strlen(s);
  if (slen > m_metadata.first)
    return false;
  return memcmp(c_str() + m_metadata.first - slen, s, slen) == 0;
}

bool String::ends_with(const String &s) const {
  return ends_with(s.c_str());
}

bool String::contains(const char *s) const {
  return find(s) != NPOS;
}

bool String::contains(const String &s) const {
  return find(s.c_str()) != NPOS;
}

String &String::trim() {
  if (!m_data || m_metadata.first == 0)
    return *this;
  size_t start = 0;
  while (start < m_metadata.first && isspace((unsigned char)m_data.ptr()[start]))
    ++start;
  size_t end = m_metadata.first;
  while (end > start && isspace((unsigned char)m_data.ptr()[end - 1]))
    --end;
  size_t new_len = end - start;
  if (start > 0)
    memmove(m_data.ptr(), m_data.ptr() + start, new_len);
  m_metadata.first = new_len;
  m_data.ptr()[new_len] = '\0';
  return *this;
}

String String::to_upper() const {
  String result(*this);
  for (size_t i = 0; i < result.m_metadata.first; ++i)
    result.m_data.ptr()[i] = (char)toupper((unsigned char)result.m_data.ptr()[i]);
  return result;
}

String String::to_lower() const {
  String result(*this);
  for (size_t i = 0; i < result.m_metadata.first; ++i)
    result.m_data.ptr()[i] = (char)tolower((unsigned char)result.m_data.ptr()[i]);
  return result;
}

String &String::erase(size_t pos, size_t count) {
  if (pos >= m_metadata.first)
    return *this;
  size_t available = m_metadata.first - pos;
  size_t actual = (count == NPOS || count > available) ? available : count;
  memmove(m_data.ptr() + pos, m_data.ptr() + pos + actual,
          m_metadata.first - pos - actual);
  m_metadata.first -= actual;
  m_data.ptr()[m_metadata.first] = '\0';
  return *this;
}

String &String::insert(size_t pos, const char *s) {
  if (!s || pos > m_metadata.first)
    return *this;
  size_t slen = strlen(s);
  if (slen == 0)
    return *this;
  if (ensure_capacity(m_metadata.first + slen + 1).is_error())
    return *this;
  memmove(m_data.ptr() + pos + slen, m_data.ptr() + pos,
          m_metadata.first - pos + 1);
  memcpy(m_data.ptr() + pos, s, slen);
  m_metadata.first += slen;
  return *this;
}

String &String::replace(size_t pos, size_t count, const char *s) {
  erase(pos, count);
  insert(pos, s);
  return *this;
}

} // namespace text
} // namespace fk
