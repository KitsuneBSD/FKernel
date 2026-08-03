#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class MessageCookie {
  uint32_t m_value{0};

public:
  constexpr MessageCookie() = default;
  constexpr explicit MessageCookie(uint32_t v) : m_value(v) {}

  constexpr uint32_t value() const { return m_value; }

  constexpr bool operator==(const MessageCookie &o) const { return m_value == o.m_value; }
  constexpr bool operator!=(const MessageCookie &o) const { return m_value != o.m_value; }
};

} // namespace fk
