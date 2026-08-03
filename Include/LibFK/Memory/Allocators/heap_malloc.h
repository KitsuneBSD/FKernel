#pragma once

#include <LibC/stddef.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
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
