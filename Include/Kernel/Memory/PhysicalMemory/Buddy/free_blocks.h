#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents a free block in the buddy memory allocator.
 */
struct FreeBlock {
  uintptr_t phys_addr;
  FreeBlock *next;
};
