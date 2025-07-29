#pragma once

#include <LibC/stddef.h>

inline void* operator new(LibC::size_t, void* ptr) noexcept
{
    return ptr;
}

inline void operator delete(void*, void*) noexcept
{
}

inline void* operator new[](LibC::size_t, void* ptr) noexcept { return ptr; }
inline void operator delete[](void*, void*) noexcept { }
