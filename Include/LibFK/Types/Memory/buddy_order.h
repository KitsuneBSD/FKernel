#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class BuddyOrder {
public:
  constexpr BuddyOrder() : m_order(0) {}
  constexpr explicit BuddyOrder(uint32_t order) : m_order(order) {}

  uint32_t value() const { return m_order; }
  size_t page_count() const { return size_t(1) << m_order; }
  size_t byte_count() const { return page_count() * 4096; }

  bool operator==(BuddyOrder o) const { return m_order == o.m_order; }
  bool operator!=(BuddyOrder o) const { return m_order != o.m_order; }
  bool operator<(BuddyOrder o) const  { return m_order < o.m_order; }
  bool operator>(BuddyOrder o) const  { return m_order > o.m_order; }

private:
  uint32_t m_order;
};

} // namespace fk
