#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

namespace MemoryManagement {

void FreeListAllocator::initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept
{
    FK::alert_if_f(initialized == true, "FreeListAllocator: Already initialized");

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

void* FreeListAllocator::alloc_zeroed(LibC::size_t size, LibC::size_t alignment) noexcept
{
    void* ptr = alloc(size, alignment);
    if (ptr)
        LibC::memset(ptr, 0, size);
    return ptr;
}

void* FreeListAllocator::alloc(LibC::size_t size, LibC::size_t alignment) noexcept
{
    FK::enforcef(initialized, "FreeListAllocator: Alloc called before initialization");
    FK::enforcef(size != 0, "FreeListAllocator: Alloc called with size 0");
    FK::enforcef((alignment & (alignment - 1)) == 0, "FreeListAllocator: Alignment must be power of two");

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        FreeMemoryBlock* current = &(*it);
        LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(current);

        LibC::uintptr_t header_addr = block_addr;
        LibC::uintptr_t user_data_addr = (header_addr + sizeof(BlockHeader) + alignment - 1) & ~(alignment - 1);
        LibC::size_t padding = user_data_addr - (header_addr + sizeof(BlockHeader));
        LibC::size_t total_needed = sizeof(BlockHeader) + padding + size;

        if (current->size >= total_needed) {
            if (current->size > total_needed + sizeof(FreeMemoryBlock)) {
                LibC::uintptr_t next_block_addr = block_addr + total_needed;
                auto* next_block = reinterpret_cast<FreeMemoryBlock*>(next_block_addr);
                next_block->size = current->size - total_needed;

                free_list.remove(current);
                free_list.append(next_block);
            } else {
                free_list.remove(current);
                total_needed = current->size;
            }

            auto* header = reinterpret_cast<BlockHeader*>(header_addr);
            header->size = total_needed;
            header->padding = padding;
            header->magic_check = BlockHeader::magic;

            return reinterpret_cast<void*>(user_data_addr);
        }
    }

    Log(LogLevel::WARN, "FreeListAllocator: Out of memory");
    return nullptr;
}

void FreeListAllocator::free(void* ptr) noexcept
{
    FK::enforcef(ptr != nullptr, "FreeListAllocator: Free called with null pointer");

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
        return reinterpret_cast<LibC::uintptr_t>(a) < reinterpret_cast<LibC::uintptr_t>(b);
    });

    try_coalesce(block);
}

void FreeListAllocator::remove_block(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: remove_block received nullptr");
    free_list.remove(block);
}

void FreeListAllocator::try_coalesce(FreeMemoryBlock* block)
{
    FK::enforcef(block != nullptr, "FreeListAllocator: try_coalesce received nullptr");

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(block);

    if (block->ListNode.next) {
        auto* next = block->ListNode.next;
        LibC::uintptr_t next_addr = reinterpret_cast<LibC::uintptr_t>(next);

        if (block_addr + block->size == next_addr) {
            block->size += next->size;
            remove_block(next);
        }
    }

    if (block->ListNode.prev) {
        auto* prev = block->ListNode.prev;
        LibC::uintptr_t prev_addr = reinterpret_cast<LibC::uintptr_t>(prev);

        if (prev_addr + prev->size == block_addr) {
            prev->size += block->size;
            remove_block(block);
            block = prev;
        }
    }
}

LibC::size_t FreeListAllocator::remaining() const noexcept
{
    LibC::size_t total = 0;
    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        total += it->size;
    }
    return total;
}

}
