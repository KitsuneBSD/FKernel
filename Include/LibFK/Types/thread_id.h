#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class ThreadId {
public:
  static constexpr uint64_t INVALID = ~uint64_t(0);

  constexpr ThreadId() : m_id(INVALID) {}
  constexpr explicit ThreadId(uint64_t id) : m_id(id) {}

  uint64_t value() const { return m_id; }
  bool is_valid() const { return m_id != INVALID; }

  bool operator==(ThreadId o) const { return m_id == o.m_id; }
  bool operator!=(ThreadId o) const { return m_id != o.m_id; }
  bool operator<(ThreadId o) const  { return m_id < o.m_id; }

private:
  uint64_t m_id;
};

} // namespace fk
