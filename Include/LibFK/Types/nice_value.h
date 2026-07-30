#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class NiceValue {
  int8_t m_value{0};

public:
  static constexpr int8_t MIN = -20;
  static constexpr int8_t MAX = 19;

  constexpr NiceValue() = default;
  constexpr explicit NiceValue(int8_t v) : m_value(v) {}

  constexpr int8_t value() const { return m_value; }

  constexpr bool operator==(const NiceValue &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const NiceValue &o) const { return m_value != o.m_value; }
  constexpr bool operator<(const NiceValue &o) const { return m_value < o.m_value; }
  constexpr bool operator>(const NiceValue &o) const { return m_value > o.m_value; }
};

} // namespace fk
