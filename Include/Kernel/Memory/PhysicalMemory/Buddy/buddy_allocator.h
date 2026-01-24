#pragma once

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif 
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>
#include <LibFK/Algorithms/log.h>
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

  /** @brief Allocates a new node from the state's pool. */
  FreeBlock* new_block(uintptr_t phys);

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
   * @brief Adds a new range to the allocator and re-initializes.
   */
  void add_range(uintptr_t base_address, size_t length);

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
};
