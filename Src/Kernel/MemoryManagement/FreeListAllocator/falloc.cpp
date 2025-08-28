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
    if (FK::alert_if(!initialized, "FreeListAllocator: Alloc before initialization"))
        return nullptr;
    if (FK::alert_if(size == 0, "FreeListAllocator: Alloc with size 0"))
        return nullptr;
    if (FK::alert_if((alignment & (alignment - 1)) != 0, "FreeListAllocator: Alignment must be power of two"))
        return nullptr;

    return try_allocate(size, alignment);
}

void FreeListAllocator::free(void* ptr)
{
    if (FK::alert_if_f(ptr == nullptr, "FreeListAllocator: Free called with null"))
        return;

    LibC::uintptr_t user_data_addr = reinterpret_cast<LibC::uintptr_t>(ptr);
    LibC::uintptr_t header_addr = user_data_addr - sizeof(BlockHeader);
    auto* header = reinterpret_cast<BlockHeader*>(header_addr);

    if (FK::alert_if_f(!header->is_valid(), "FreeListAllocator: Invalid free: corrupt or double free"))
        return;

    LibC::uintptr_t block_addr = header_addr - header->padding;

    if (FK::alert_if_f(block_addr < heap_start || block_addr >= heap_end,
            "FreeListAllocator: Block outside heap bounds"))
        return;

    auto* block = reinterpret_cast<FreeMemoryBlock*>(block_addr);
    block->size = header->size;

    insert_block(block);
    try_coalesce(block);
}

void FreeListAllocator::insert_block(FreeMemoryBlock* block)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: insert_block received nullptr")) {
        return;
    }

    if (free_list.is_empty()) {
        free_list.append(block);
        return;
    }

    free_list.insert_ordered(block, [](auto* a, auto* b) {
        if (FK::alert_if_f(a == nullptr || b == nullptr, "FreeListAllocator: insert_ordered comparator received nullptr")) {
            return false; // abort comparison safely
        }
        return reinterpret_cast<LibC::uintptr_t>(a) < reinterpret_cast<LibC::uintptr_t>(b);
    });

    try_coalesce(block);
}

void FreeListAllocator::remove_block(FreeMemoryBlock* block)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: remove_block received nullptr")) {
        return;
    }

    if (FK::alert_if_f(!block->ListNode.is_linked(), "FreeListAllocator: remove_block called on unlinked node")) {
        return;
    }

    free_list.remove(block);
}

void FreeListAllocator::try_coalesce(FreeMemoryBlock* block)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: try_coalesce received nullptr")) {
        return;
    }

    coalesce_with_next(block);
    coalesce_with_prev(block);
}

void FreeListAllocator::coalesce_with_next(FreeMemoryBlock* block)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: coalesce_with_next received nullptr")) {
        return;
    }

    auto* next = block->ListNode.next;
    if (FK::alert_if_f(next == nullptr, "FreeListAllocator: coalesce_with_next next block is nullptr")) {
        return;
    }

    LibC::uintptr_t block_end = reinterpret_cast<LibC::uintptr_t>(block) + block->size;
    LibC::uintptr_t next_addr = reinterpret_cast<LibC::uintptr_t>(next);

    if (block_end == next_addr) {
        block->size += next->size;
        remove_block(next);
    }
}

void FreeListAllocator::coalesce_with_prev(FreeMemoryBlock* block)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: coalesce_with_prev received nullptr")) {
        return;
    }

    auto* prev = block->ListNode.prev;
    if (FK::alert_if_f(prev == nullptr, "FreeListAllocator: coalesce_with_prev prev block is nullptr")) {
        return;
    }

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
    if (FK::alert_if_f(!initialized, "FreeListAllocator: try_allocate called before initialization")) {
        return nullptr;
    }
    if (FK::alert_if_f(size == 0, "FreeListAllocator: try_allocate with size 0")) {
        return nullptr;
    }
    if (FK::alert_if_f((alignment & (alignment - 1)) != 0, "FreeListAllocator: try_allocate alignment must be power of two")) {
        return nullptr;
    }

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        FreeMemoryBlock* current = &(*it);
        if (FK::alert_if_f(current == nullptr, "FreeListAllocator: try_allocate found nullptr block")) {
            continue;
        }

        if (can_fit_block(current, size, alignment))
            return place_block_for(current, size, alignment);
    }

    Log(LogLevel::WARN, "FreeListAllocator: Out of memory");

    return nullptr;
}

bool FreeListAllocator::can_fit_block(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment) const
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: can_fit_block received nullptr"))
        return false;

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);
    LibC::uintptr_t header_addr = block_addr;
    LibC::uintptr_t user_data_addr = (header_addr + sizeof(BlockHeader) + alignment - 1) & ~(alignment - 1);
    LibC::size_t padding = user_data_addr - (header_addr + sizeof(BlockHeader));
    LibC::size_t total_needed = sizeof(BlockHeader) + padding + size;

    return block->size >= total_needed;
}

void* FreeListAllocator::place_block_for(FreeMemoryBlock* block, LibC::size_t size, LibC::size_t alignment)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: place_block_for received nullptr"))
        return nullptr;

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);

    LibC::uintptr_t aligned_user_data = (block_addr + sizeof(BlockHeader) + alignment - 1) & ~(alignment - 1);
    LibC::uintptr_t header_addr = aligned_user_data - sizeof(BlockHeader);
    LibC::size_t padding = header_addr - block_addr;

    LibC::size_t total_needed = padding + sizeof(BlockHeader) + size;

    if (FK::alert_if_f(block->size < total_needed, "FreeListAllocator: place_block_for block too small"))
        return nullptr;

    handle_split_or_take_block(block, total_needed);

    auto* header = reinterpret_cast<BlockHeader*>(header_addr);
    header->size = total_needed;
    header->padding = padding;
    header->magic_check = BlockHeader::magic;

    return reinterpret_cast<void*>(aligned_user_data);
}

void FreeListAllocator::handle_split_or_take_block(FreeMemoryBlock* block, LibC::size_t total_needed)
{
    if (FK::alert_if_f(block == nullptr, "FreeListAllocator: handle_split_or_take_block received nullptr"))
        return;

    LibC::size_t remaining = block->size - total_needed;
    bool should_split = remaining > sizeof(FreeMemoryBlock);

    if (should_split) {
        LibC::uintptr_t next_block_addr = reinterpret_cast<LibC::uintptr_t>(block) + total_needed;
        if (FK::alert_if_f(next_block_addr + sizeof(FreeMemoryBlock) > heap_end,
                "FreeListAllocator: next block out of heap bounds"))
            return;

        auto* next_block = reinterpret_cast<FreeMemoryBlock*>(next_block_addr);
        next_block->size = remaining;

        remove_block(block);
        insert_block(next_block);
    } else {
        remove_block(block);
    }
}

}
