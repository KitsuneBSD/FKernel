#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents a free block in the buddy memory allocator.
 *
 * FreeBlock nodes are stored inline inside the free physical page itself
 * (at KERNEL_VIRT_BASE + phys_addr).  `list_idx` records which free-list the
 * node currently belongs to so that remove() can reject stale lookups
 * (a page that is free at order N must not be unlinked from order M > N).
 */
struct FreeBlock {
  uintptr_t phys_addr;
  size_t list_idx;
  FreeBlock *next;
  FreeBlock *prev;  // doubly-linked for O(1) remove
};
