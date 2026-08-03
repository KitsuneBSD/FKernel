#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class TaskPriority {
  uint8_t m_value{0};

public:
  static constexpr uint8_t MIN = 0;
  static constexpr uint8_t MAX = 255;

  constexpr TaskPriority() = default;
  constexpr explicit TaskPriority(uint8_t v) : m_value(v) {}

  constexpr uint8_t value() const { return m_value; }

  constexpr bool operator==(const TaskPriority &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const TaskPriority &o) const { return m_value != o.m_value; }
  constexpr bool operator<(const TaskPriority &o) const { return m_value < o.m_value; }
  constexpr bool operator>(const TaskPriority &o) const { return m_value > o.m_value; }
  constexpr bool operator<=(const TaskPriority &o) const { return m_value <= o.m_value; }
  constexpr bool operator>=(const TaskPriority &o) const { return m_value >= o.m_value; }
};

} // namespace fk
