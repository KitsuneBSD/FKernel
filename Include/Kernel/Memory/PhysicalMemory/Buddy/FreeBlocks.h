#pragma once 

/* 
* Usually, a double linked list can be used to easily coalesce free blocks in buddy memory allocation.
* Can link on previous block and a next block
*/

/**
 * @brief Represents a free block in the buddy memory allocator.
 */
struct FreeBlock {
    FreeBlock* prev;    
    FreeBlock* next;
};