#pragma once

#include <LibFK/Types/types.h>

/**
 * @class SlabAllocator
 * @brief Object-caching slab allocator for fixed-size kernel allocations.
 *
 * Manages multiple caches of common sizes (16–2048 bytes).  Each cache
 * grows by allocating page-sized slabs from the kernel heap, subdividing
 * them into fixed-size objects linked by an embedded free list.  Alloc/free
 * is O(1).  Fallback: passes requests above the largest cache size through
 * to the kernel heap (kmalloc/kfree).
 */
class SlabAllocator {
  bool m_is_initialized{false};

  SlabAllocator() = default;
  SlabAllocator(const SlabAllocator &) = delete;
  SlabAllocator &operator=(const SlabAllocator &) = delete;

public:
  static SlabAllocator &the() {
    static SlabAllocator inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }
  void initialize();

  void *allocate(size_t size);
  bool deallocate(void *ptr);
};
