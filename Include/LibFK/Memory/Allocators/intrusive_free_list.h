#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace fk {
namespace memory {

// Intrusive free list for fixed-size object allocators (e.g. slab allocator).
// Objects are stored in a forward-linked list using the first sizeof(void*)
// bytes of each slot as the next pointer — no separate metadata required.
class IntrusiveFreeList {
    void*  m_head{nullptr};
    size_t m_count{0};

public:
    IntrusiveFreeList() = default;

    void initialize(uint8_t* object_base, size_t object_size, size_t count) {
        m_head  = nullptr;
        m_count = count;
        for (size_t i = 0; i < count; ++i) {
            void* obj = object_base + i * object_size;
            *reinterpret_cast<void**>(obj) = m_head;
            m_head = obj;
        }
    }

    void* pop() {
        if (!m_head) return nullptr;
        void* obj = m_head;
        m_head = *reinterpret_cast<void**>(obj);
        --m_count;
        return obj;
    }

    void push(void* ptr) {
        *reinterpret_cast<void**>(ptr) = m_head;
        m_head = ptr;
        ++m_count;
    }

    size_t count()    const { return m_count; }
    bool   is_empty() const { return m_count == 0; }
};

} // namespace memory
} // namespace fk
