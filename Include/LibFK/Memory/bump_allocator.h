#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fk {
namespace memory {

class BumpAllocator {
public:
    explicit BumpAllocator(size_t capacity) : m_capacity(capacity) {
        m_base = static_cast<uint8_t*>(kmalloc(capacity));
        m_cursor = m_base;
    }

    ~BumpAllocator() {
        if (m_base) kfree(m_base);
    }

    void* allocate(size_t size, size_t align = alignof(max_align_t)) {
        if (!m_base) return nullptr;
        uintptr_t cur = reinterpret_cast<uintptr_t>(m_cursor);
        uintptr_t aligned = (cur + align - 1) & ~(align - 1);
        if (aligned + size > reinterpret_cast<uintptr_t>(m_base) + m_capacity)
            return nullptr;
        m_cursor = reinterpret_cast<uint8_t*>(aligned + size);
        m_used = static_cast<size_t>(m_cursor - m_base);
        return reinterpret_cast<void*>(aligned);
    }

    void free_all() {
        m_cursor = m_base;
        m_used = 0;
    }

    size_t used() const { return m_used; }
    size_t capacity() const { return m_capacity; }
    bool is_valid() const { return m_base != nullptr; }

    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

private:
    uint8_t* m_base{nullptr};
    uint8_t* m_cursor{nullptr};
    size_t m_capacity{0};
    size_t m_used{0};
};

} // namespace memory

using memory::BumpAllocator;

} // namespace fk
