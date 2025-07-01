#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace MemoryManagement {

struct alignas(16) BlockHeader {
    LibC::size_t size;
    LibC::size_t padding;
    LibC::uintptr_t magic_check;

    static constexpr LibC::uintptr_t magic = 0xDEADBEEFDEADBEEFull;

    bool is_valid() const noexcept
    {
        return magic_check == magic;
    }
};

struct FreeMemoryBlock {
    LibC::size_t size;
    FreeMemoryBlock* next;
    FreeMemoryBlock* prev;
};

class FreeListAllocator {
private:
    LibC::uintptr_t heap_start = 0;
    LibC::uintptr_t heap_end = 0;
    FreeMemoryBlock* free_list_head = nullptr;
    bool initialized = false;

    FreeListAllocator() = default;

    void insert_block(FreeMemoryBlock* block);
    void remove_block(FreeMemoryBlock* block);
    void try_coalesce(FreeMemoryBlock* block);

public:
    static FreeListAllocator& instance() noexcept
    {
        static FreeListAllocator s_instance;
        return s_instance;
    }

    void initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept;

    void* alloc(LibC::size_t size, LibC::size_t alignment = 8) noexcept;
    void* alloc_zeroed(LibC::size_t size, LibC::size_t alignment = 8) noexcept;
    void free(void* ptr) noexcept;

    LibC::size_t remaining() const noexcept;
};

}
inline void* Falloc(LibC::size_t size, LibC::size_t alignment = 8)
{
    return MemoryManagement::FreeListAllocator::instance().alloc(size, alignment);
}

inline void* Falloc_zeroed(LibC::size_t size, LibC::size_t alignment = 8)
{
    return MemoryManagement::FreeListAllocator::instance().alloc_zeroed(size, alignment);
}

inline void Ffree(void* ptr)
{
    MemoryManagement::FreeListAllocator::instance().free(ptr);
}
