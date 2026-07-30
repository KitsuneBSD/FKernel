#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class MessageId {
  uint32_t m_value{0};

public:
  constexpr MessageId() = default;
  constexpr explicit MessageId(uint32_t v) : m_value(v) {}

  constexpr uint32_t value() const { return m_value; }

  constexpr bool operator==(const MessageId &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const MessageId &o) const { return m_value != o.m_value; }
};

} // namespace fk
