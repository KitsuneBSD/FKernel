#pragma once

#include <LibFK/Types/types.h>

class SlabFreeList {
  void *m_head{nullptr};
  size_t m_count{0};

public:
  SlabFreeList() = default;

  void initialize(uint8_t *object_base, size_t object_size, size_t count) {
    m_head = nullptr;
    m_count = count;

    for (size_t i = 0; i < count; i++) {
      void *obj = object_base + i * object_size;
      *reinterpret_cast<void **>(obj) = m_head;
      m_head = obj;
    }
  }

  void *pop() {
    if (!m_head) return nullptr;
    void *obj = m_head;
    m_head = *reinterpret_cast<void **>(obj);
    m_count--;
    return obj;
  }

  void push(void *ptr) {
    *reinterpret_cast<void **>(ptr) = m_head;
    m_head = ptr;
    m_count++;
  }

  size_t count() const { return m_count; }
  bool is_empty() const { return m_count == 0; }
  bool is_full() const { return m_count == 0; }
};
