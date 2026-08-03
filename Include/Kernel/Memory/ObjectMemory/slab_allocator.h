#pragma once

#include <LibFK/Synchronization/spinlock.h>
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
 *
 * All slab operations are IRQ-safe (ScopedLockIRQ) because they may run from
 * interrupt context (drivers allocate in ISRs) and across CPUs (M6).
 */
class SlabAllocator {
  bool m_is_initialized{false};
  mutable fk::synchronization::Spinlock m_lock;

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

  /**
   * @brief Returns whether @p ptr belongs to a slab cache.
   *
   * Used by the kernel heap to route realloc/free of slab objects and to
   * avoid treating a slab pointer as a heap BlockHeader.
   */
  bool is_slab_allocation(void *ptr) const;

  /**
   * @brief Grows (or shrinks) a slab-backed allocation.
   *
   * Returns @p ptr unchanged when the new size fits in the object, a new
   * object otherwise (old contents copied, old object freed).  Returns
   * nullptr when @p ptr is not a slab allocation or on out-of-memory.
   */
  void *reallocate(void *ptr, size_t size);

private:
  // Lock-free internals.  Public entry points hold m_lock exactly once so the
  // helpers can call each other without re-entrant locking (Spinlock only
  // supports recursion on a non-zero CPU id, which host test builds lack).
  void *allocate_locked(size_t size);
  bool deallocate_locked(void *ptr);
  bool is_slab_allocation_locked(void *ptr) const;
  void *reallocate_locked(void *ptr, size_t size);
};
