#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class GroupId {
  uint32_t m_value{0};

public:
  static constexpr uint32_t ROOT = 0;

  constexpr GroupId() = default;
  constexpr explicit GroupId(uint32_t v) : m_value(v) {}

  constexpr uint32_t value() const { return m_value; }

  constexpr bool is_root() const { return m_value == ROOT; }

  constexpr bool operator==(const GroupId &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const GroupId &o) const { return m_value != o.m_value; }
};

} // namespace fk
