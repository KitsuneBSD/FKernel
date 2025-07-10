#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

namespace MemoryManagement {

void FreeListAllocator::initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept
{
    if (initialized) {
        Log(LogLevel::WARN, "[FreeListAllocator] Already initialized.");
        return;
    }

    heap_start = start;
    heap_end = end;

    free_list = {};

    auto* initial_block = reinterpret_cast<FreeMemoryBlock*>(start);
    initial_block->size = end - start;

    free_list.append(initial_block);

    initialized = true;

    LibC::size_t size_in_bytes = end - start;
    Logf(LogLevel::INFO, "[FreeListAllocator] Initialized: start=0x%lx, end=0x%lx, size=%lu bytes (%lu MiB)",
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
    if (!initialized || size == 0) {
        Log(LogLevel::ERROR, "[FreeListAllocator] Alloc called before initialization or with size 0.");
        return nullptr;
    }

    if ((alignment & (alignment - 1)) != 0) {
        Log(LogLevel::ERROR, "[FreeListAllocator] Alignment must be power of two.");
        return nullptr;
    }

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
                FreeMemoryBlock* next_block = reinterpret_cast<FreeMemoryBlock*>(next_block_addr);
                next_block->size = current->size - total_needed;

                free_list.remove(current);
                free_list.append(next_block);

            } else {
                free_list.remove(current);
                total_needed = current->size;
            }

            BlockHeader* header = reinterpret_cast<BlockHeader*>(header_addr);
            header->size = total_needed;
            header->padding = padding;
            header->magic_check = BlockHeader::magic;

            void* user_ptr = reinterpret_cast<void*>(user_data_addr);
            return user_ptr;
        }
    }

    Log(LogLevel::WARN, "[FreeListAllocator] Out of memory.");
    return nullptr;
}

void FreeListAllocator::free(void* ptr) noexcept
{
    if (!ptr) {
        Log(LogLevel::ERROR, "[FreeListAllocator] Free called with null pointer.");
        return;
    }

    LibC::uintptr_t user_addr = reinterpret_cast<LibC::uintptr_t>(ptr);
    BlockHeader* header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader));
    header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader) - header->padding);

    if (!header->is_valid()) {
        Log(LogLevel::ERROR, "[FreeListAllocator] Invalid free: mismatch. Possible corruption or double free.");
        return;
    }

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(header);
    FreeMemoryBlock* block = reinterpret_cast<FreeMemoryBlock*>(block_addr);

    block->size = header->size;

    insert_block(block);
    try_coalesce(block);
}

void FreeListAllocator::insert_block(FreeMemoryBlock* block)
{
    if (free_list.is_empty()) {
        free_list.append(block);
        return;
    }

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        FreeMemoryBlock* current = &(*it);

        if (block < current) {

            auto& block_node = block->ListNode;
            auto& current_node = current->ListNode;

            block_node.next = current;
            block_node.prev = current_node.prev;

            if (current_node.prev) {
                (current_node.prev)->ListNode.next = block;
            } else {
                free_list.set_head(block);
            }

            current_node.prev = block;

            return;
        }
    }

    free_list.append(block);
}

void FreeListAllocator::remove_block(FreeMemoryBlock* block)
{
    free_list.remove(block);
}

void FreeListAllocator::try_coalesce(FreeMemoryBlock* block)
{
    auto* next = (block->ListNode.next);
    if (next && (reinterpret_cast<LibC::uintptr_t>(block) + block->size == reinterpret_cast<LibC::uintptr_t>(next))) {
        block->size += next->size;
        free_list.remove(next);
    }

    auto* prev = (block->ListNode.prev);
    if (prev && (reinterpret_cast<LibC::uintptr_t>(prev) + prev->size == reinterpret_cast<LibC::uintptr_t>(block))) {
        prev->size += block->size;
        free_list.remove(block);
    }
}

LibC::size_t FreeListAllocator::remaining() const noexcept
{
    LibC::size_t total = 0;
    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        FreeMemoryBlock* block = &(*it);
        total += block->size;
    }
    return total;
}

} // namespace MemoryManagement
