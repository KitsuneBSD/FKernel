#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class ColorValue {
  uint32_t m_value{0};

public:
  constexpr ColorValue() = default;
  constexpr explicit ColorValue(uint32_t v) : m_value(v) {}

  constexpr uint32_t value() const { return m_value; }

  constexpr bool operator==(const ColorValue &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const ColorValue &o) const { return m_value != o.m_value; }
};

} // namespace fk
