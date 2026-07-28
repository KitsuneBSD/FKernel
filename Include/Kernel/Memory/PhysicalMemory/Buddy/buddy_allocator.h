#pragma once

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif 
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/aligner.h>

/**
 * @class BuddyAllocator
 * @brief Manages physical memory blocks using the buddy system algorithm.
 * 
 * The buddy allocator handles power-of-two sized block allocations and 
 * merges free neighbors (buddies) to reduce fragmentation.
 */
class BuddyAllocator {
private:
  BuddyState m_state;

  uintptr_t m_base_address;
  size_t m_length;

protected:
  /**
   * @brief Internal initialization routine to populate free lists.
   */
  void initialize();

  /** @brief Converts an order to a list index. */
  size_t order_to_index(size_t order) const;

  /** @brief Calculates the buddy address of a block. */
  uintptr_t buddy_of(uintptr_t address, size_t order) const;

  /** @brief Checks if an address is within the managed range. */
  bool in_range(uintptr_t address) const;

  /** @brief Pushes a block into the specified order's free list. */
  void push_free_block(size_t order, uintptr_t address);

  /** @brief Pops a block from the specified order's free list. */
  uintptr_t pop_free_block(size_t order);

public:
  BuddyAllocator();

  /**
   * @brief Constructs an allocator with a specific memory range.
   */
  BuddyAllocator(uintptr_t base_address, size_t length);

  /**
   * @brief Stores the managed range without populating free lists.
   */
  void add_range(uintptr_t base_address, size_t length);

  /**
   * @brief Initializes free lists from bitmap, only adding blocks of truly free pages.
   * @param bitmap The per-page allocation bitmap for the zone.
   * @param zone_base Physical base address of the zone (for frame index calculation).
   */
  void initialize_from_bitmap(const fk::containers::Bitmap<uint64_t>& bitmap, uintptr_t zone_base);

  /**
   * @brief Allocates a block of memory of the specified order.
   * @param order Power-of-two order (12 = 4KB, 21 = 2MB).
   * @return Pointer to allocated physical memory or nullptr.
   */
  void* alloc(size_t order);

  /**
   * @brief Frees a previously allocated block and attempts to merge buddies.
   */
  void free(void* ptr, size_t order);

  /**
   * @brief Removes a single page from all buddy free lists so it won't be
   *        returned by alloc() after the bitmap has already given it out.
   */
  void invalidate_page(uintptr_t phys);
};
