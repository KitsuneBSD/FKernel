#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

namespace MemoryManagement {

void FreeListAllocator::initialize(LibC::uintptr_t start, LibC::uintptr_t end)
{
    FK::alert_if_f(initialized, "FreeListAllocator: Already initialized");
    FK::enforcef(start < end, "FreeListAllocator: invalid initialization range");

    heap_start = start;
    heap_end = end;
    free_list = {};

    auto* initial_block = reinterpret_cast<FreeMemoryBlock*>(start);
    initial_block->size = end - start;

    free_list.append(initial_block);
    initialized = true;

    LibC::size_t size_in_bytes = end - start;
    Logf(LogLevel::INFO, "FreeListAllocator: Initialized: start=0x%lx, end=0x%lx, size=%lu bytes (%lu MiB)",
        start, end, size_in_bytes, size_in_bytes / FK::MiB);
}

void* FreeListAllocator::alloc_zeroed(LibC::size_t size, LibC::size_t alignment)
{
    void* ptr = alloc(size, alignment);
    if (ptr)
        LibC::memset(ptr, 0, size);
    return ptr;
}

void* FreeListAllocator::alloc(LibC::size_t size, LibC::size_t alignment)
{
    FK::enforcef(initialized, "FreeListAllocator: Alloc before initialization");
    FK::enforcef(size != 0, "FreeListAllocator: Alloc with size 0");
    FK::enforcef((alignment & (alignment - 1)) == 0, "FreeListAllocator: Alignment must be power of two");

    return try_allocate(size, alignment);
}

void FreeListAllocator::free(void* ptr)
{
    FK::enforcef(ptr != nullptr, "FreeListAllocator: Free called with null");

    LibC::uintptr_t user_addr = reinterpret_cast<LibC::uintptr_t>(ptr);
    auto* header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader));
    header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader) - header->padding);

    FK::enforcef(header->is_valid(), "FreeListAllocator: Invalid free: header corrupt or double free");

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(header);
    auto* block = reinterpret_cast<FreeMemoryBlock*>(block_addr);
    block->size = header->size;

    insert_block(block);
    try_coalesce(block);
}

void FreeListAllocator::insert_block(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: insert_block received nullptr");

    if (free_list.is_empty()) {
        free_list.append(block);
        return;
    }

    free_list.insert_ordered(block, [](auto* a, auto* b) {
        FK::enforcef(a != nullptr && b != nullptr, "FreeListAllocator: insert_ordered comparator received nullptr");
        return reinterpret_cast<LibC::uintptr_t>(a) < reinterpret_cast<LibC::uintptr_t>(b);
    });

    try_coalesce(block);
}

void FreeListAllocator::remove_block(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: remove_block received nullptr");
    FK::enforcef(block->ListNode.is_linked(), "FreeListAllocator: remove_block called on unlinked node");
    free_list.remove(block);
}

void FreeListAllocator::try_coalesce(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: try_coalesce received nullptr");

    coalesce_with_next(block);
    coalesce_with_prev(block);
}

void FreeListAllocator::coalesce_with_next(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: coalesce_with_next received nullptr");

    auto* next = block->ListNode.next;
    FK::alert_if_f(next == nullptr, "FreeListAllocator: coalesce_with_next next block is nullptr");
    if (next == nullptr)
        return;

    LibC::uintptr_t block_end = reinterpret_cast<LibC::uintptr_t>(block) + block->size;
    LibC::uintptr_t next_addr = reinterpret_cast<LibC::uintptr_t>(next);

    if (block_end == next_addr) {
        block->size += next->size;
        remove_block(next);
    }
}

void FreeListAllocator::coalesce_with_prev(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: coalesce_with_prev received nullptr");

    auto* prev = block->ListNode.prev;
    FK::alert_if_f(prev == nullptr, "FreeListAllocator: coalesce_with_prev prev block is nullptr");
    if (prev == nullptr)
        return;

    LibC::uintptr_t prev_end = reinterpret_cast<LibC::uintptr_t>(prev) + prev->size;
    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);

    if (prev_end == block_addr) {
        prev->size += block->size;
        remove_block(block);
        block = prev;
    }
}

void* FreeListAllocator::try_allocate(LibC::size_t size, LibC::size_t alignment)
{
    FK::enforcef(initialized, "FreeListAllocator: try_allocate called before initialization");
    FK::enforcef(size != 0, "FreeListAllocator: try_allocate with size 0");
    FK::enforcef((alignment & (alignment - 1)) == 0, "FreeListAllocator: try_allocate alignment must be power of two");

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        FreeMemoryBlock* current = &(*it);
        FK::enforcef(current != nullptr, "FreeListAllocator: try_allocate found nullptr block");

        if (can_fit_block(current, size, alignment))
            return place_block_for(current, size, alignment);
    }

    Log(LogLevel::WARN, "FreeListAllocator: Out of memory");
    return nullptr;
}

bool FreeListAllocator::can_fit_block(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment) const
{
    FK::enforcef(block != nullptr, "FreeListAllocator: can_fit_block received nullptr");

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);
    LibC::uintptr_t header_addr = block_addr;
    LibC::uintptr_t user_data_addr = (header_addr + sizeof(BlockHeader) + alignment - 1) & ~(alignment - 1);
    LibC::size_t padding = user_data_addr - (header_addr + sizeof(BlockHeader));
    LibC::size_t total_needed = sizeof(BlockHeader) + padding + size;

    return block->size >= total_needed;
}

void* FreeListAllocator::place_block_for(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: place_block_for received nullptr");

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);
    LibC::uintptr_t header_addr = block_addr;
    LibC::uintptr_t user_data_addr = (header_addr + sizeof(BlockHeader) + alignment - 1) & ~(alignment - 1);
    LibC::size_t padding = user_data_addr - (header_addr + sizeof(BlockHeader));
    LibC::size_t total_needed = sizeof(BlockHeader) + padding + size;

    FK::enforcef(block->size >= total_needed, "FreeListAllocator: place_block_for block too small");

    handle_split_or_take_block(block, total_needed);

    auto* header = reinterpret_cast<BlockHeader*>(header_addr);
    header->size = total_needed;
    header->padding = padding;
    header->magic_check = BlockHeader::magic;

    return reinterpret_cast<void*>(user_data_addr);
}

void FreeListAllocator::handle_split_or_take_block(FreeMemoryBlock* block, LibC::size_t total_needed)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: handle_split_or_take_block received nullptr");

    LibC::size_t remaining = block->size - total_needed;
    bool should_split = remaining > sizeof(FreeMemoryBlock);

    if (should_split) {
        LibC::uintptr_t next_block_addr = reinterpret_cast<LibC::uintptr_t>(block) + total_needed;
        FK::enforcef(next_block_addr + sizeof(FreeMemoryBlock) <= heap_end,
            "FreeListAllocator: next block out of heap bounds");

        auto* next_block = reinterpret_cast<FreeMemoryBlock*>(next_block_addr);
        next_block->size = remaining;

        remove_block(block);
        insert_block(next_block);
    } else {
        remove_block(block);
    }
}

}
