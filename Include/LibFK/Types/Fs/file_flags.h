#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class FileFlags {
public:
  constexpr FileFlags() : m_flags(0) {}
  constexpr explicit FileFlags(int flags) : m_flags(flags) {}

  int value() const { return m_flags; }

  bool has(int flag) const { return (m_flags & flag) != 0; }

  FileFlags with(int flag) const { return FileFlags(m_flags | flag); }
  FileFlags without(int flag) const { return FileFlags(m_flags & ~flag); }

  bool operator==(FileFlags o) const { return m_flags == o.m_flags; }
  bool operator!=(FileFlags o) const { return m_flags != o.m_flags; }

private:
  int m_flags;
};

} // namespace fk
