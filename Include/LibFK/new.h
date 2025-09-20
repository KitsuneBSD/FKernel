#pragma once

#include <LibFK/heap_malloc.h>

/**
 * @brief Placement new: construct object at a given memory location.
 * @param size Size of the object (ignored)
 * @param ptr Pointer to pre-allocated memory
 * @return Pointer to the object memory (same as ptr)
 */
void *operator new(size_t, void *ptr) noexcept { return ptr; }

/**
 * @brief Placement new for arrays.
 * @param size Size of the array (ignored)
 * @param ptr Pointer to pre-allocated memory
 * @return Pointer to the array memory (same as ptr)
 */
void *operator new[](size_t, void *ptr) noexcept { return ptr; }

/**
 * @brief Allocate memory for a single object using kmalloc.
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory
 */
void *operator new(size_t size) { return kmalloc(size); }

/**
 * @brief Allocate memory for an array using kmalloc.
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory
 */
void *operator new[](size_t size) { return kmalloc(size); }

/**
 * @brief Free memory allocated by operator new.
 * @param ptr Pointer to memory to free
 */
void operator delete(void *ptr) { kfree(ptr); }

/**
 * @brief Free memory allocated for arrays.
 * @param ptr Pointer to memory to free
 */
void operator delete[](void *ptr) { kfree(ptr); }

/**
 * @brief Delete with size information (C++14), forwards to kfree.
 * @param ptr Pointer to memory
 * @param size Size of allocation (ignored)
 */
void operator delete(void *ptr, size_t) { kfree(ptr); }

/**
 * @brief Delete array with size information, forwards to kfree.
 * @param ptr Pointer to memory
 * @param size Size of allocation (ignored)
 */
void operator delete[](void *ptr, size_t) { kfree(ptr); }

/**
 * @brief Nothrow placement delete (called if constructor throws)
 */
void operator delete(void *, void *) noexcept {}

/**
 * @brief Nothrow placement delete for arrays.
 */
void operator delete[](void *, void *) noexcept {}
