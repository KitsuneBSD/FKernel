#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class StackPointer {
  uint64_t m_value{0};

public:
  constexpr StackPointer() = default;
  constexpr explicit StackPointer(uint64_t v) : m_value(v) {}

  constexpr uint64_t value() const { return m_value; }

  constexpr bool is_null() const { return m_value == 0; }

  constexpr bool operator==(const StackPointer &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const StackPointer &o) const { return m_value != o.m_value; }
};

} // namespace fk
