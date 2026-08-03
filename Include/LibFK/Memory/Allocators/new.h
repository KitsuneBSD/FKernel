#pragma once

#include <LibFK/Types/types.h>

/**
 * Placement new (construct object at given memory location).
 */
inline void *operator new(size_t, void *ptr) noexcept { return ptr; }
inline void *operator new[](size_t, void *ptr) noexcept { return ptr; }

/**
 * Global new/delete operators (declared only).
 */
void *operator new(size_t size);
void *operator new[](size_t size);

void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;

void operator delete(void *ptr, size_t) noexcept;
void operator delete[](void *ptr, size_t) noexcept;

/**
 * Placement delete (called if constructor throws).
 */
inline void operator delete(void *, void *) noexcept {}
inline void operator delete[](void *, void *) noexcept {}
