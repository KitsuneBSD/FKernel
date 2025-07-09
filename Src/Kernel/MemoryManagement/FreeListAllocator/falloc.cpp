#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
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

    free_list_head = reinterpret_cast<FreeMemoryBlock*>(start);
    free_list_head->size = end - start;
    free_list_head->next = nullptr;
    free_list_head->prev = nullptr;

    initialized = true;

    LibC::size_t size_in_bytes = end - start;

    Logf(LogLevel::INFO, "[FreeListAllocator] Initialized: start=0x%lx, end=0x%lx, size=%lu bytes (%lu MiB)",
        start, end, size_in_bytes, size_in_bytes / FK::MiB);
}

void* FreeListAllocator::alloc_zeroed(LibC::size_t size, LibC::size_t alignment) noexcept
{
    void* ptr = alloc(size, alignment);
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

    FreeMemoryBlock* current = free_list_head;

    while (current) {
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
                next_block->next = current->next;
                next_block->prev = current->prev;

                if (next_block->next)
                    next_block->next->prev = next_block;

                if (next_block->prev)
                    next_block->prev->next = next_block;
                else
                    free_list_head = next_block;
            } else {
                remove_block(current);
                total_needed = current->size;
            }

            BlockHeader* header = reinterpret_cast<BlockHeader*>(header_addr);
            header->size = total_needed;
            header->padding = padding;
            header->magic_check = BlockHeader::magic;

            void* user_ptr = reinterpret_cast<void*>(user_data_addr);

            return user_ptr;
        }

        current = current->next;
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
    BlockHeader* header = nullptr;

    header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader));
    header = reinterpret_cast<BlockHeader*>(user_addr - sizeof(BlockHeader) - header->padding);

    if (header->is_valid()) {
        Log(LogLevel::ERROR, "[FreeListAllocator] Invalid free:  mismatch. Possible corruption or double free.");
        return;
    }

    LibC::uintptr_t block_addr = reinterpret_cast<LibC::uintptr_t>(header);
    FreeMemoryBlock* block = reinterpret_cast<FreeMemoryBlock*>(block_addr);

    block->size = header->size;
    block->next = nullptr;
    block->prev = nullptr;

    insert_block(block);
    try_coalesce(block);
}

void FreeListAllocator::insert_block(FreeMemoryBlock* block)
{
    if (!free_list_head) {
        free_list_head = block;
        block->next = nullptr;
        block->prev = nullptr;
        return;
    }

    FreeMemoryBlock* current = free_list_head;
    while (current && current < block)
        current = current->next;

    if (current) {
        block->next = current;
        block->prev = current->prev;

        if (current->prev)
            current->prev->next = block;
        else
            free_list_head = block;

        current->prev = block;
    } else {
        FreeMemoryBlock* tail = free_list_head;
        while (tail->next)
            tail = tail->next;

        tail->next = block;
        block->prev = tail;
        block->next = nullptr;
    }
}

void FreeListAllocator::remove_block(FreeMemoryBlock* block)
{
    if (block->prev)
        block->prev->next = block->next;
    else
        free_list_head = block->next;

    if (block->next)
        block->next->prev = block->prev;
}

void FreeListAllocator::try_coalesce(FreeMemoryBlock* block)
{
    if (block->next && reinterpret_cast<LibC::uintptr_t>(block) + block->size == reinterpret_cast<LibC::uintptr_t>(block->next)) {
        block->size += block->next->size;
        remove_block(block->next);
    }

    if (block->prev && reinterpret_cast<LibC::uintptr_t>(block->prev) + block->prev->size == reinterpret_cast<LibC::uintptr_t>(block)) {
        block->prev->size += block->size;
        remove_block(block);
    }
}

LibC::size_t FreeListAllocator::remaining() const noexcept
{
    LibC::size_t total = 0;
    FreeMemoryBlock* current = free_list_head;
    while (current) {
        total += current->size;
        current = current->next;
    }
    return total;
}

} // namespace MemoryManagement
