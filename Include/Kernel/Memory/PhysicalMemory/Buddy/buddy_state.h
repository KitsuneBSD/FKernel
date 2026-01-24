#pragma once

#include <LibFK/Types/types.h>

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>

/**
 * @struct BuddyState
 * @brief Internal state for the buddy allocator, managing nodes and lists.
 */
struct BuddyState {
    FreeBlock m_block_pool[16384]; ///< Static pool of block nodes to avoid recursion in PMM.
    FreeBlock* m_free_lists[NUM_ORDERS]; ///< Heads of the free lists for each order.
    size_t m_block_index; ///< Current index in the static node pool.

    /** @brief Resets all free lists and the node index. */
    void reset();

    /** @brief Allocates a node from the static pool. */
    FreeBlock* allocate_node(uintptr_t phys);

    /** @brief Pushes a block node onto the specified list index. */
    void push(size_t idx, FreeBlock* block);

    /** @brief Pops a block node from the specified list index. */
    FreeBlock* pop(size_t idx);

    /** @brief Removes a specific physical address from a free list. */
    bool remove(size_t idx, uintptr_t phys);
};
