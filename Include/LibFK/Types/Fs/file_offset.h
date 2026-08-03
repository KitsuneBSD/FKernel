#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class FileOffset {
public:
  constexpr FileOffset() : m_offset(0) {}
  constexpr explicit FileOffset(int64_t offset) : m_offset(offset) {}

  int64_t value() const { return m_offset; }
  bool is_negative() const { return m_offset < 0; }

  FileOffset operator+(FileOffset o) const { return FileOffset(m_offset + o.m_offset); }
  FileOffset operator-(FileOffset o) const { return FileOffset(m_offset - o.m_offset); }

  bool operator==(FileOffset o) const { return m_offset == o.m_offset; }
  bool operator!=(FileOffset o) const { return m_offset != o.m_offset; }
  bool operator<(FileOffset o) const  { return m_offset < o.m_offset; }
  bool operator>(FileOffset o) const  { return m_offset > o.m_offset; }

private:
  int64_t m_offset;
};

} // namespace fk
