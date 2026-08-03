#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class NotificationBits {
  uint64_t m_value{0};

public:
  constexpr NotificationBits() = default;
  constexpr explicit NotificationBits(uint64_t v) : m_value(v) {}

  constexpr uint64_t value() const { return m_value; }

  constexpr bool is_empty() const { return m_value == 0; }
  bool has(uint32_t bit) const { return (m_value >> bit) & 1; }
  void set(uint32_t bit) { m_value |= (1ULL << bit); }
  void clear_all() { m_value = 0; }
  NotificationBits &operator|=(NotificationBits other) { m_value |= other.m_value; return *this; }

  constexpr bool operator==(const NotificationBits &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const NotificationBits &o) const { return m_value != o.m_value; }
  constexpr NotificationBits operator|(NotificationBits other) const { return NotificationBits(m_value | other.m_value); }
  constexpr NotificationBits operator&(NotificationBits other) const { return NotificationBits(m_value & other.m_value); }
  constexpr NotificationBits operator~() const { return NotificationBits(~m_value); }
};

} // namespace fk
