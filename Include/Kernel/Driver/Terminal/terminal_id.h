#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace terminal {

class TerminalId {
  uint32_t m_value;

public:
  explicit TerminalId(uint32_t id) : m_value(id) {}

  bool is_valid() const { return m_value > 0; }
  uint32_t value() const { return m_value; }

  bool operator==(const TerminalId& other) const { return m_value == other.m_value; }
  bool operator!=(const TerminalId& other) const { return !(*this == other); }
};

} // namespace terminal
} // namespace fkernel
