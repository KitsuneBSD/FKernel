#pragma once

#include <LibC/stddef.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

extern "C" {
    void* heap_malloc(size_t size);
    void heap_free(void* ptr);
    void* kmalloc(size_t size);
    void kfree(void* ptr);
    void* kcalloc(size_t nmemb, size_t size);
    void* krealloc(void* ptr, size_t size);
}

namespace fk {
namespace memory {

/**
 * @struct BlockHeader
 * @brief Header for each memory block in the heap.
 * 
 * Each block is 16-byte aligned.
 */
struct alignas(16) BlockHeader {
    size_t size;            ///< Size of the usable memory area (excluding header).
    bool is_free;           ///< Whether the block is available for allocation.
    BlockHeader* next;      ///< Pointer to the next block in the linked list.
    static constexpr uint32_t MAGIC = 0xC0FFEE; ///< Magic number for corruption detection.
    uint32_t magic;         ///< Block's magic number.
};

/**
 * @struct Allocator
 * @brief Simple linked-list based heap allocator.
 */
struct Allocator {
    BlockHeader* head = nullptr; ///< Head of the block linked list.
    bool m_initialized = false;  ///< Whether the allocator has been initialized.

    /** @brief Initializes the allocator using the kernel heap memory range. */
    void initialize();

    /** @brief Allocates a memory block of the specified size. */
    void* allocate(size_t size);

    /** @brief Frees a previously allocated memory block. */
    void free(void* ptr);
};

Allocator& heap_allocator();

// Result-based interface (Raw bytes)
fk::core::Result<uint64_t*, fk::core::Error> allocate(size_t size);
fk::core::Result<uint64_t*, fk::core::Error> reallocate(uint64_t* ptr, size_t size);
void free(uint64_t* ptr);

/**
 * @brief Freestanding allocation function for typed objects.
 * @tparam T Type to allocate
 * @return Pointer to allocated memory, or nullptr if allocation fails
 */
template <typename T> inline T *allocate() {
  void *ptr = kmalloc(sizeof(T));
  if (!ptr) {
    fk::algorithms::kdebug("HEAP_ALLOCATE", "OutOfMemory for size %zu", sizeof(T));
    return nullptr;
  }
  return reinterpret_cast<T *>(ptr);
}

/**
 * @brief Freestanding deallocation function for typed objects.
 * @tparam T Type to deallocate
 * @param ptr Pointer to memory to free
 */
template <typename T> inline void deallocate(T *ptr) {
  if (ptr)
    kfree(ptr);
}

} // namespace memory
} // namespace fk
