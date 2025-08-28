#pragma once

#include "Kernel/MemoryManagement/FreeListAllocator/falloc.h"
#include "LibC/stddef.h"
namespace LibC {

template<typename T>
constexpr T min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
constexpr T max(T a, T b)
{
    return (a > b) ? a : b;
}

}

inline void* malloc(LibC::size_t size, LibC::size_t alignment = 16)
{
    void* ptr = Falloc(size, alignment);
    if (ptr)
        return ptr;
    return nullptr;
}

inline void free(void* ptr)
{
    if (!ptr)
        return;

    Ffree(ptr);
}

inline void* calloc(LibC::size_t nmemb, LibC::size_t size)
{
    LibC::size_t total = nmemb * size; // Sem overflow check, simplificado
    void* ptr = malloc(total);
    if (ptr)
        LibC::memset(ptr, 0, total);
    return ptr;
}

inline void* realloc(void* old_ptr, LibC::size_t new_size)
{
    if (!old_ptr)
        return malloc(new_size);

    if (new_size == 0) {
        free(old_ptr);
        return nullptr;
    }

    void* new_ptr = malloc(new_size);
    if (!new_ptr)
        return nullptr;

    LibC::memcpy(new_ptr, old_ptr, new_size);
    free(old_ptr);
    return new_ptr;
}
