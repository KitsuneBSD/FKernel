#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class MlqfLevel {
  uint8_t m_value{0};

public:
  constexpr MlqfLevel() = default;
  constexpr explicit MlqfLevel(uint8_t v) : m_value(v) {}

  constexpr uint8_t value() const { return m_value; }

  constexpr bool operator==(const MlqfLevel &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const MlqfLevel &o) const { return m_value != o.m_value; }
  constexpr bool operator<(const MlqfLevel &o) const { return m_value < o.m_value; }
  constexpr bool operator>(const MlqfLevel &o) const { return m_value > o.m_value; }
};

} // namespace fk
