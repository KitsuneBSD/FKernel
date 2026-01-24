#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fk {
namespace memory {

extern "C" uint8_t __heap_start[];
extern "C" uint8_t __heap_end[];

/**
 * @brief Returns the global kernel heap allocator.
 */
Allocator& heap_allocator() {
    return s_allocator;
}

void Allocator::initialize() {
    if (m_initialized) return;

    uintptr_t start_addr = (reinterpret_cast<uintptr_t>(__heap_start) + 15) & ~15;
    uintptr_t end_addr = reinterpret_cast<uintptr_t>(__heap_end) & ~15;
    size_t total_size = end_addr - start_addr;

    ASSERT(total_size > sizeof(BlockHeader));

    head = reinterpret_cast<BlockHeader*>(start_addr);
    head->size = total_size - sizeof(BlockHeader);
    head->is_free = true;
    head->next = nullptr;
    head->magic = BlockHeader::MAGIC;

    m_initialized = true;
    fk::algorithms::klog("HEAP", "Initialized Simple Linked List Allocator. Size: %zu bytes", total_size);
}

static inline uint64_t save_and_disable_interrupts() {
    uint64_t flags;
    asm volatile("pushfq ; popq %0 ; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void restore_interrupts(uint64_t flags) {
    asm volatile("pushq %0 ; popfq" : : "g"(flags) : "memory");
}

void* Allocator::allocate(size_t size) {
    if (!m_initialized) return nullptr;

    uint64_t flags = save_and_disable_interrupts();

    // Align size to 16 bytes (required for many x86_64 structures and SSE)
    size = (size + 15) & ~15;

    BlockHeader* current = head;
    while (current) {
        if (current->magic != BlockHeader::MAGIC) {
            fk::algorithms::kerror("HEAP", "Heap Corruption at %p! Magic: 0x%lx Expected: 0x%lx", 
                                  current, (uint64_t)current->magic, (uint64_t)BlockHeader::MAGIC);
            fk::algorithms::kerror("HEAP", "Next: %p, Size: %zu, Free: %s", 
                                  current->next, current->size, current->is_free ? "yes" : "no");
            while(1) asm volatile("hlt");
        }

        if (current->is_free && current->size >= size) {
            // Can we split this block? 
            if (current->size >= size + sizeof(BlockHeader) + 16) {
                BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<uint8_t*>(current) + sizeof(BlockHeader) + size
                );
                
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = current->next;
                new_block->magic = BlockHeader::MAGIC;

                current->size = size;
                current->next = new_block;
            }

            current->is_free = false;
            void* ptr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(current) + sizeof(BlockHeader));
            memset(ptr, 0, size);
            
            restore_interrupts(flags);
            return ptr;
        }
        current = current->next;
    }

    restore_interrupts(flags);
    fk::algorithms::kerror("HEAP", "Out of memory! Requested: %zu", size);
    return nullptr;
}

void Allocator::free(void* ptr) {
    if (!ptr) return;

    uint64_t flags = save_and_disable_interrupts();

    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader)
    );

    if (header->magic != BlockHeader::MAGIC) {
        fk::algorithms::kerror("HEAP", "Double free or corruption at %p! (Magic: %p)", ptr, (void*)(uint64_t)header->magic);
        restore_interrupts(flags);
        while(1) asm volatile("hlt");
        return;
    }

    if (header->is_free) {
        fk::algorithms::kerror("HEAP", "Block at %p is already free!", ptr);
        restore_interrupts(flags);
        return;
    }

    header->is_free = true;
    
    // Simple zeroing for debugging
    memset(ptr, 0xDE, header->size > 64 ? 64 : header->size);

    // Merge with next
    if (header->next && header->next->is_free) {
        if (header->next->magic == BlockHeader::MAGIC) {
            header->size += sizeof(BlockHeader) + header->next->size;
            header->next = header->next->next;
        }
    }

    restore_interrupts(flags);
}

// Wrapper implementation
fk::core::Result<uint64_t*, fk::core::Error> allocate(size_t size) {
    void* ptr = s_allocator.allocate(size);
    if (!ptr) return fk::core::Error::OutOfMemory;
    return reinterpret_cast<uint64_t*>(ptr);
}

fk::core::Result<uint64_t*, fk::core::Error> reallocate(uint64_t* ptr, size_t size) {
    if (!ptr) return allocate(size);
    if (size == 0) {
        s_allocator.free(ptr);
        return nullptr;
    }

    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader)
    );
    
    // Safety check before access
    if (header->magic != BlockHeader::MAGIC) return fk::core::Error::InvalidData;

    size_t old_size = header->size;
    if (size <= old_size) return ptr;

    auto new_res = allocate(size);
    if (new_res.is_error()) return new_res;

    uint64_t* new_ptr = new_res.value();
    memcpy(new_ptr, ptr, old_size);
    s_allocator.free(ptr);
    
    return new_ptr;
}

void free(uint64_t* ptr) {
    s_allocator.free(ptr);
}

} // namespace memory
} // namespace fk

extern "C" {
    void* heap_malloc(size_t size) { return fk::memory::s_allocator.allocate(size); }
    void heap_free(void* ptr) { fk::memory::s_allocator.free(ptr); }
    void* kmalloc(size_t size) { return fk::memory::s_allocator.allocate(size); }
    void kfree(void* ptr) { fk::memory::s_allocator.free(ptr); }
    
    void* kcalloc(size_t nmemb, size_t size) {
        void* ptr = kmalloc(nmemb * size);
        // Note: allocate() already zeroes out memory in our impl, 
        // but for correctness/robustness if allocate changed:
        // if (ptr) memset(ptr, 0, nmemb * size); 
        return ptr;
    }
    
    void* krealloc(void* ptr, size_t size) {
        auto res = fk::memory::reallocate(reinterpret_cast<uint64_t*>(ptr), size);
        if (res.is_error()) return nullptr;
        return res.value();
    }
}
