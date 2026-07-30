#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class CpuAffinity {
  uint64_t m_value{0};

public:
  constexpr CpuAffinity() = default;
  constexpr explicit CpuAffinity(uint64_t v) : m_value(v) {}

  constexpr uint64_t value() const { return m_value; }

  constexpr bool is_any() const { return m_value == 0; }
  constexpr bool includes(uint32_t cpu) const { return (m_value >> cpu) & 1; }

  constexpr bool operator==(const CpuAffinity &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const CpuAffinity &o) const { return m_value != o.m_value; }
};

} // namespace fk
