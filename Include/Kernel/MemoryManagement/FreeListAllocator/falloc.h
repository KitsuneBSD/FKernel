#pragma once

#include "LibFK/intrusiveList.h"
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace MemoryManagement {

struct BlockHeader {
    static constexpr LibC::uint32_t magic = 0xDEADBEEF;
    LibC::size_t size;
    LibC::size_t padding;
    LibC::uint32_t magic_check;

    bool is_valid() const
    {
        return magic_check == magic;
    }
};

struct FreeMemoryBlock {
    FK::IntrusiveNode<FreeMemoryBlock> ListNode;
    LibC::size_t size;
};

class FreeListAllocator {
private:
    LibC::uintptr_t heap_start = 0;
    LibC::uintptr_t heap_end = 0;

    FK::IntrusiveList<FreeMemoryBlock, &FreeMemoryBlock::ListNode> free_list;
    bool initialized = false;

    FreeListAllocator() = default;

    void insert_block(FreeMemoryBlock* block);
    void remove_block(FreeMemoryBlock* block);
    void try_coalesce(FreeMemoryBlock* block);
    void* try_allocate(LibC::size_t size, LibC::size_t alignment);
    void handle_split_or_take_block(FreeMemoryBlock* current, LibC::size_t total_needed);
    void* place_block(FreeMemoryBlock* block, LibC::size_t padding, LibC::size_t total_needed, LibC::uintptr_t user_data_addr);
    void* place_block_for(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment);
    bool can_fit_block(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment) const;

    void coalesce_with_next(FreeMemoryBlock* block);
    void coalesce_with_prev(FreeMemoryBlock* block);

public:
    static FreeListAllocator& instance()
    {
        static FreeListAllocator s_instance;
        return s_instance;
    }

    void initialize(LibC::uintptr_t start, LibC::uintptr_t end);
    void* alloc(LibC::size_t size, LibC::size_t alignment = 8);
    void* alloc_zeroed(LibC::size_t size, LibC::size_t alignment = 8);
    void free(void* ptr);
    LibC::size_t remaining() const;
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
