#pragma once

#include <LibFK/Types/types.h>

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>

struct BuddyState {
    FreeBlock m_block_pool[16384];
    FreeBlock* m_free_lists[NUM_ORDERS];
    size_t m_block_index;

    void reset();
    FreeBlock* allocate_node(uintptr_t phys);
    void push(size_t idx, FreeBlock* block);
    FreeBlock* pop(size_t idx);
    bool remove(size_t idx, uintptr_t phys);
};
