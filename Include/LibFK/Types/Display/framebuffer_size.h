#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class FramebufferSize {
  size_t m_value{0};

public:
  constexpr FramebufferSize() = default;
  constexpr explicit FramebufferSize(size_t v) : m_value(v) {}

  constexpr size_t value() const { return m_value; }

  constexpr bool is_zero() const { return m_value == 0; }

  constexpr bool operator==(const FramebufferSize &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const FramebufferSize &o) const { return m_value != o.m_value; }
  constexpr bool operator<(const FramebufferSize &o) const { return m_value < o.m_value; }
  constexpr bool operator>(const FramebufferSize &o) const { return m_value > o.m_value; }
};

} // namespace fk
