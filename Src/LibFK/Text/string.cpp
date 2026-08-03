#include <LibC/ctype.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace text {

// ---- construction / destruction -------------------------------------------

String::String() : m_data(), m_metadata({0, 0}) {
  m_data.sso[0] = '\0'; // empty SSO string
}

String::String(const char *s) : m_data(), m_metadata({0, 0}) {
  if (s) _copy_from_cstring(s, strlen(s));
  else   m_data.sso[0] = '\0';
}

String::String(const String &other) : m_data(), m_metadata({0, 0}) {
  if (_copy_from_string(other).is_error())
    m_data.sso[0] = '\0';
}

String::String(String &&other) noexcept : m_data(), m_metadata(other.m_metadata) {
  if (other.m_is_sso()) {
    memcpy(m_data.sso, other.m_data.sso, other.m_metadata.first + 1);
  } else {
    m_data.heap_ptr = other.m_data.heap_ptr;
    other.m_data.heap_ptr = nullptr;
  }
  other.m_metadata = {0, 0};
  other.m_data.sso[0] = '\0';
}

String::~String() {
  if (!m_is_sso() && m_data.heap_ptr)
    kfree(m_data.heap_ptr);
}

// ---- assignment -----------------------------------------------------------

String &String::operator=(const String &other) {
  if (this != &other) _copy_from_string(other);
  return *this;
}

String &String::operator=(String &&other) noexcept {
  if (this == &other) return *this;
  if (!m_is_sso() && m_data.heap_ptr) kfree(m_data.heap_ptr);
  m_metadata = other.m_metadata;
  if (other.m_is_sso()) {
    memcpy(m_data.sso, other.m_data.sso, other.m_metadata.first + 1);
  } else {
    m_data.heap_ptr = other.m_data.heap_ptr;
    other.m_data.heap_ptr = nullptr;
  }
  other.m_metadata = {0, 0};
  other.m_data.sso[0] = '\0';
  return *this;
}

String &String::operator=(const char *s) {
  _copy_from_cstring(s ? s : "", strlen(s ? s : ""));
  return *this;
}

// ---- capacity / access ----------------------------------------------------

void String::clear() {
  m_metadata.first = 0;
  m_data_ptr()[0] = '\0';
}

fk::core::Result<void, fk::core::Error> String::reserve(size_t new_cap) {
  return ensure_capacity(new_cap);
}

char &String::operator[](size_t index) {
  return m_data_ptr()[index];
}

const char &String::operator[](size_t index) const {
  return m_data_ptr()[index];
}

// ---- append ---------------------------------------------------------------

String &String::append(const char *s) {
  if (s) _append_data(s, strlen(s));
  return *this;
}

String &String::append(const String &str) {
  if (str.m_metadata.first > 0)
    _append_data(str.m_data_ptr(), str.m_metadata.first);
  return *this;
}

void String::push_back(char c) {
  if (ensure_capacity(m_metadata.first + 2).is_error()) return;
  char* dst = m_data_ptr();
  dst[m_metadata.first++] = c;
  dst[m_metadata.first]   = '\0';
}

// ---- private helpers ------------------------------------------------------

fk::core::Result<void, fk::core::Error> String::ensure_capacity(size_t min_cap) {
  // SSO: fits inline — no allocation needed
  if (min_cap <= SSO_CAP + 1 && m_is_sso()) return fk::core::Result<void>();

  size_t cur_cap = m_metadata.second;
  if (cur_cap >= min_cap) return fk::core::Result<void>(); // heap already big enough

  size_t new_cap = (cur_cap < 32) ? 32 : cur_cap * 2;
  if (new_cap < min_cap) new_cap = min_cap;

  char *new_ptr = static_cast<char*>(kmalloc(new_cap + 1));
  if (!new_ptr) return fk::core::Error::OutOfMemory;

  size_t len = m_metadata.first;
  if (len > 0) memcpy(new_ptr, m_data_ptr(), len);
  new_ptr[len] = '\0';

  if (!m_is_sso() && m_data.heap_ptr) kfree(m_data.heap_ptr);
  m_data.heap_ptr  = new_ptr;
  m_metadata.second = new_cap;
  return fk::core::Result<void>();
}

fk::core::Result<void, fk::core::Error> String::_copy_from_cstring(const char *s, size_t len) {
  if (!s) { clear(); return fk::core::Result<void>(); }

  auto res = ensure_capacity(len + 1);
  if (res.is_error()) return res.error();

  char* dst = m_data_ptr();
  memcpy(dst, s, len);
  dst[len] = '\0';
  m_metadata.first = len;
  return fk::core::Result<void>();
}

fk::core::Result<void, fk::core::Error> String::_copy_from_string(const String &other) {
  return _copy_from_cstring(other.m_data_ptr(), other.m_metadata.first);
}

fk::core::Result<void, fk::core::Error> String::_append_data(const char *data, size_t len) {
  if (!data || len == 0) return fk::core::Result<void>();

  auto res = ensure_capacity(m_metadata.first + len + 1);
  if (res.is_error()) return res.error();

  char* dst = m_data_ptr();
  memcpy(dst + m_metadata.first, data, len);
  m_metadata.first += len;
  dst[m_metadata.first] = '\0';
  return fk::core::Result<void>();
}

// ---- search ---------------------------------------------------------------

String String::substr(size_t pos, size_t count) const {
  if (pos >= m_metadata.first) return String();
  size_t available = m_metadata.first - pos;
  size_t len = (count == NPOS || count > available) ? available : count;
  String result;
  if (result._append_data(m_data_ptr() + pos, len).is_error()) return String();
  return result;
}

size_t String::find(char c, size_t pos) const {
  const char* p = m_data_ptr();
  for (size_t i = pos; i < m_metadata.first; ++i)
    if (p[i] == c) return i;
  return NPOS;
}

size_t String::find(const char *s, size_t pos) const {
  if (!s) return NPOS;
  size_t slen = strlen(s);
  if (slen == 0) return pos;
  const char* p = m_data_ptr();
  for (size_t i = pos; i + slen <= m_metadata.first; ++i)
    if (memcmp(p + i, s, slen) == 0) return i;
  return NPOS;
}

size_t String::find(const String &s, size_t pos) const { return find(s.c_str(), pos); }

size_t String::rfind(char c, size_t pos) const {
  if (m_metadata.first == 0) return NPOS;
  const char* p = m_data_ptr();
  size_t i = (pos == NPOS || pos >= m_metadata.first) ? m_metadata.first - 1 : pos;
  while (true) {
    if (p[i] == c) return i;
    if (i == 0) break;
    --i;
  }
  return NPOS;
}

size_t String::rfind(const char *s, size_t pos) const {
  if (!s) return NPOS;
  size_t slen = strlen(s);
  if (slen == 0) return m_metadata.first;
  const char* p = m_data_ptr();
  size_t start = (pos == NPOS || pos + slen > m_metadata.first) ? m_metadata.first - slen : pos;
  for (size_t i = start + 1; i-- > 0;)
    if (memcmp(p + i, s, slen) == 0) return i;
  return NPOS;
}

bool String::starts_with(const char *s) const {
  if (!s) return true;
  size_t slen = strlen(s);
  return slen <= m_metadata.first && memcmp(m_data_ptr(), s, slen) == 0;
}

bool String::starts_with(const String &s) const { return starts_with(s.c_str()); }

bool String::ends_with(const char *s) const {
  if (!s) return true;
  size_t slen = strlen(s);
  if (slen > m_metadata.first) return false;
  return memcmp(m_data_ptr() + m_metadata.first - slen, s, slen) == 0;
}

bool String::ends_with(const String &s) const { return ends_with(s.c_str()); }

bool String::contains(const char *s) const { return find(s) != NPOS; }
bool String::contains(const String &s) const { return find(s.c_str()) != NPOS; }

// ---- mutation -------------------------------------------------------------

String &String::trim() {
  if (m_metadata.first == 0) return *this;
  char* p = m_data_ptr();
  size_t start = 0;
  while (start < m_metadata.first && isspace((unsigned char)p[start])) ++start;
  size_t end = m_metadata.first;
  while (end > start && isspace((unsigned char)p[end - 1])) --end;
  size_t new_len = end - start;
  if (start > 0) memmove(p, p + start, new_len);
  m_metadata.first = new_len;
  p[new_len] = '\0';
  return *this;
}

String String::to_upper() const {
  String result(*this);
  char* p = result.m_data_ptr();
  for (size_t i = 0; i < result.m_metadata.first; ++i)
    p[i] = (char)toupper((unsigned char)p[i]);
  return result;
}

String String::to_lower() const {
  String result(*this);
  char* p = result.m_data_ptr();
  for (size_t i = 0; i < result.m_metadata.first; ++i)
    p[i] = (char)tolower((unsigned char)p[i]);
  return result;
}

String &String::erase(size_t pos, size_t count) {
  if (pos >= m_metadata.first) return *this;
  char* p = m_data_ptr();
  size_t available = m_metadata.first - pos;
  size_t actual = (count == NPOS || count > available) ? available : count;
  memmove(p + pos, p + pos + actual, m_metadata.first - pos - actual);
  m_metadata.first -= actual;
  p[m_metadata.first] = '\0';
  return *this;
}

String &String::insert(size_t pos, const char *s) {
  if (!s || pos > m_metadata.first) return *this;
  size_t slen = strlen(s);
  if (slen == 0) return *this;
  if (ensure_capacity(m_metadata.first + slen + 1).is_error()) return *this;
  char* p = m_data_ptr();
  memmove(p + pos + slen, p + pos, m_metadata.first - pos + 1);
  memcpy(p + pos, s, slen);
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
