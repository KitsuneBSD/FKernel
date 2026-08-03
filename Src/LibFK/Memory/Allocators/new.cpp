/**
 * @file new.cpp
 * @brief Global operator new and delete overrides for the kernel heap.
 */

#include <LibC/stddef.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

/// @brief Allocates memory for a single object.
void *operator new(size_t size) { return heap_malloc(size); }

/// @brief Allocates memory for an array of objects.
void *operator new[](size_t size) { return heap_malloc(size); }

/// @brief Frees memory for a single object.
void operator delete(void *ptr) noexcept { heap_free(ptr); }

/// @brief Frees memory for an array of objects.
void operator delete[](void *ptr) noexcept { heap_free(ptr); }

/// @brief Sized delete (C++14).
void operator delete(void *ptr, size_t) noexcept { heap_free(ptr); }

/// @brief Sized delete for arrays (C++14).
void operator delete[](void *ptr, size_t) noexcept { heap_free(ptr); }
