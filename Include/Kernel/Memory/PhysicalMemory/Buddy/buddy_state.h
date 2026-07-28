#pragma once

#include <LibFK/Types/types.h>

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>

/**
 * @struct BuddyState
 * @brief Internal state for the buddy allocator, managing free lists.
 *
 * Free blocks are stored inline in the freed pages themselves via the
 * kernel's higher-half direct map (KERNEL_VIRT_BASE + phys).  No static
 * node pool is needed.
 */
struct BuddyState {
    FreeBlock* m_free_lists[NUM_ORDERS];

    void reset();

    void push(size_t idx, uintptr_t phys);

    uintptr_t pop(size_t idx);

    bool remove(size_t idx, uintptr_t phys);
};
