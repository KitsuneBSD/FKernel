#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class FrameIndex {
public:
  static constexpr uint64_t INVALID = ~uint64_t(0);

  constexpr FrameIndex() : m_index(INVALID) {}
  constexpr explicit FrameIndex(uint64_t index) : m_index(index) {}

  uint64_t value() const { return m_index; }
  uintptr_t to_physical_addr() const { return m_index * 4096; }
  bool is_valid() const { return m_index != INVALID; }

  bool operator==(FrameIndex o) const { return m_index == o.m_index; }
  bool operator!=(FrameIndex o) const { return m_index != o.m_index; }
  bool operator<(FrameIndex o) const  { return m_index < o.m_index; }

private:
  uint64_t m_index;
};

} // namespace fk
