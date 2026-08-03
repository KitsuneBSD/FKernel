#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class ScreenCoord {
  int m_value{0};

public:
  constexpr ScreenCoord() = default;
  constexpr explicit ScreenCoord(int v) : m_value(v) {}

  constexpr int value() const { return m_value; }
  constexpr bool is_valid() const { return m_value >= 0; }

  constexpr bool operator==(const ScreenCoord &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const ScreenCoord &o) const { return m_value != o.m_value; }
  constexpr bool operator<(const ScreenCoord &o) const { return m_value < o.m_value; }
  constexpr bool operator>(const ScreenCoord &o) const { return m_value > o.m_value; }
  constexpr bool operator<=(const ScreenCoord &o) const { return m_value <= o.m_value; }
  constexpr bool operator>=(const ScreenCoord &o) const { return m_value >= o.m_value; }
};

} // namespace fk
