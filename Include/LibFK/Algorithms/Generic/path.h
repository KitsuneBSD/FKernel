#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Iterates the slash-separated components of a POSIX path. Empty components
// (leading, trailing, or repeated slashes) are skipped. The component
// semantics (e.g. `.` and `..`) are left to the caller.
//
//   const char* data;
//   size_t len;
//   PathTokenizer tok(path, maxlen);
//   while (tok.next_component(data, len)) { ... }
//
// `path` must be NUL-terminated within `maxlen` bytes.

class PathTokenizer {
public:
  PathTokenizer(const char* path, size_t maxlen) : m_cursor(path), m_end(path + maxlen) {}

  // Returns true and fills `out_data`/`out_len` with the next component.
  // The returned slice is NUL-terminated at out_data[out_len].
  bool next_component(const char*& out_data, size_t& out_len) {
    skip_slashes();
    if (m_cursor >= m_end || *m_cursor == '\0') return false;
    out_data = m_cursor;
    while (m_cursor < m_end && *m_cursor != '\0' && *m_cursor != '/') ++m_cursor;
    out_len = static_cast<size_t>(m_cursor - out_data);
    return true;
  }

  // True when no more components remain.
  bool exhausted() const { return at_end() || *m_cursor == '\0'; }

private:
  bool at_end() const { return m_cursor >= m_end; }

  void skip_slashes() {
    while (!at_end() && *m_cursor == '/') ++m_cursor;
  }

  const char* m_cursor;
  const char* m_end;
};

} // namespace algorithms
} // namespace fk
