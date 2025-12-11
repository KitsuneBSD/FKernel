#pragma once 

/**
 * @brief Represents a free block in the buddy memory allocator.
 */
struct FreeBlock {
    FreeBlock* next;
};