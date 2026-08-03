#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

template <typename T, size_t MAX_PENDING = 32> class AsyncIoQueue {
private:
  T m_pending_operations[MAX_PENDING];
  bool m_active[MAX_PENDING] = {false};
  uint32_t m_head = 0;
  uint32_t m_tail = 0;
  uint32_t m_count = 0;

public:
  bool enqueue(T operation) {
    if (m_count >= MAX_PENDING) {
      return false;
    }

    m_pending_operations[m_tail] = operation;
    m_active[m_tail] = true;
    m_tail = (m_tail + 1) % MAX_PENDING;
    m_count++;
    return true;
  }

  T dequeue() {
    if (m_count == 0) {
      return nullptr;
    }

    T operation = m_pending_operations[m_head];
    m_active[m_head] = false;
    m_head = (m_head + 1) % MAX_PENDING;
    m_count--;
    return operation;
  }

  T* find_pending(uint32_t slot) {
    for (uint32_t i = 0; i < MAX_PENDING; ++i) {
      if (m_active[i] && m_pending_operations[i] && m_pending_operations[i]->get_slot() == slot) {
        return &m_pending_operations[i];
      }
    }
    return nullptr;
  }

  bool is_full() const { return m_count >= MAX_PENDING; }
  bool is_empty() const { return m_count == 0; }
  uint32_t size() const { return m_count; }
};

} // namespace fkernel
