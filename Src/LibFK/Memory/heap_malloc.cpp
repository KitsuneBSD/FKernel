#include <Kernel/Memory/memory_manager.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/heap_malloc.h>

namespace fk {
namespace memory {

// Result-based interface (Raw bytes)
fk::core::Result<uint64_t*, fk::core::Error> allocate(size_t size) {
    void* ptr = MemoryManager::the().allocate(size);
    if (!ptr) return fk::core::Error::OutOfMemory;
    return reinterpret_cast<uint64_t*>(ptr);
}

fk::core::Result<uint64_t*, fk::core::Error> reallocate(uint64_t* ptr, size_t size) {
    void* new_ptr = MemoryManager::the().reallocate(ptr, size);
    if (!new_ptr && size > 0) return fk::core::Error::OutOfMemory;
    return reinterpret_cast<uint64_t*>(new_ptr);
}

void free(uint64_t* ptr) {
    MemoryManager::the().free(ptr);
}

} // namespace memory
} // namespace fk

extern "C" {
    void* heap_malloc(size_t size) { return MemoryManager::the().allocate(size); }
    void heap_free(void* ptr) { MemoryManager::the().free(ptr); }
    void* kmalloc(size_t size) { return MemoryManager::the().allocate(size); }
    void kfree(void* ptr) { MemoryManager::the().free(ptr); }
    
    void* kcalloc(size_t nmemb, size_t size) {
        size_t total = nmemb * size;
        void* ptr = kmalloc(total);
        if (ptr) memset(ptr, 0, total);
        return ptr;
    }
    
    void* krealloc(void* ptr, size_t size) {
        return MemoryManager::the().reallocate(ptr, size);
    }
}