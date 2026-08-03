#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class SignalMask {
  uint64_t m_value{0};

public:
  constexpr SignalMask() = default;
  constexpr explicit SignalMask(uint64_t v) : m_value(v) {}

  constexpr uint64_t value() const { return m_value; }

  constexpr bool has(uint32_t signo) const { return (m_value >> (signo - 1)) & 1; }

  void set(uint32_t signo) { m_value |= (1ULL << (signo - 1)); }
  void clear(uint32_t signo) { m_value &= ~(1ULL << (signo - 1)); }

  constexpr bool operator==(const SignalMask &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const SignalMask &o) const { return m_value != o.m_value; }
};

} // namespace fk
