#include <LibC/assert.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h> // Assuming this is needed for Result and Error
#include <LibFK/Container/heap_malloc.h>

// Define ASSERT_NOT_REACHED if it's not found in standard headers
#ifndef ASSERT_NOT_REACHED
#define ASSERT_NOT_REACHED(msg) \
    do { \
        fk::algorithms::kerror("ASSERT_NOT_REACHED", msg); \
        /* In a bare-metal environment, we might halt or loop */ \
        /* For now, a simple infinite loop can serve as a halt */ \
        while(1); \
    } while(0)
#endif

namespace fk {
namespace containers {

extern "C" uint64_t __heap_start[];
extern "C" uint64_t __heap_end[];

Allocator s_allocator;

Allocator &heap_allocator() { return s_allocator; }

void Allocator::initialize() {
  if (heap_allocator().initialized()) {
    fk::algorithms::kerror("HEAP MALLOC", "Allocator is already initialized!");
    return;
  }

  uint64_t *heap_start = __heap_start;
  uint64_t *heap_end = __heap_end;

  ASSERT(heap_start && heap_end && heap_end > heap_start);

  const size_t heap_size = static_cast<size_t>(heap_end - heap_start);

  size_t required = 0;
  required += ChunkAllocator<8>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<8>::capacityInBytes();
  required += ChunkAllocator<16>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<16>::capacityInBytes();
  required += ChunkAllocator<4096>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<4096>::capacityInBytes();
  required += ChunkAllocator<16384>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<16384>::capacityInBytes();

  ASSERT(heap_size >= required);
  auto space = reinterpret_cast<uint64_t *>(heap_start);

  heap_allocator().alloc8().initialize(space);
  heap_allocator().alloc16().initialize(heap_allocator().alloc8().addressAfterThisAllocator());
  heap_allocator().alloc4096().initialize(heap_allocator().alloc16().addressAfterThisAllocator());
  heap_allocator().alloc16384().initialize(heap_allocator().alloc4096().addressAfterThisAllocator());
  
  heap_allocator().initialized() = true;
}

// Modified allocate function to return Result
fk::core::Result<uint64_t *, fk::core::Error> allocate(size_t size) {
  ASSERT(size > 0);

  if (size <= 8)
    return heap_allocator().alloc8().allocate();
  if (size <= 16)
    return heap_allocator().alloc16().allocate();
  if (size <= 4096)
    return heap_allocator().alloc4096().allocate();
  if (size <= 16384)
    return heap_allocator().alloc16384().allocate();
  
  fk::algorithms::kerror("HEAP MALLOC", "Requested size %zu exceeds maximum supported allocation size.", size);
  return fk::core::Error::OutOfMemory; // Or a more specific error if available
}

// Modified allocateZeroed to handle Result
fk::core::Result<uint64_t *, fk::core::Error> allocateZeroed(size_t size) {
  ASSERT(size > 0);

  auto result = allocate(size);
  if (result.is_error()) {
      return result; // Propagate the error
  }
  // If allocation succeeded, zero it out
  memset(result.value(), 0, size);
  return result; // Return the Result containing the pointer
}

void free(uint64_t *ptr) {
  ASSERT(ptr);

  if (heap_allocator().alloc8().isInAllocator(ptr)) {
    heap_allocator().alloc8().free(ptr);
    return;
  }
  if (heap_allocator().alloc16().isInAllocator(ptr)) {
    heap_allocator().alloc16().free(ptr);
    return;
  }
  if (heap_allocator().alloc4096().isInAllocator(ptr)) {
    heap_allocator().alloc4096().free(ptr);
    return;
  }
  if (heap_allocator().alloc16384().isInAllocator(ptr)) {
    heap_allocator().alloc16384().free(ptr);
    return;
  }
  // If ptr is not found in any allocator, it might be an error or a pointer
  // from a different allocator. For now, we assert.
  ASSERT_NOT_REACHED("Pointer not found in any heap allocator.");
}

// Modified reallocate to handle Result
fk::core::Result<uint64_t *, fk::core::Error> reallocate(uint64_t *ptr, size_t size) {
  ASSERT(ptr || size > 0);

  // Helper lambda to try moving allocation within a specific pool
  auto try_move = [&](auto &pool) -> fk::core::Result<uint64_t *, fk::core::Error> {
    if (pool.isInAllocator(ptr)) {
      size_t old = pool.chunk_size();
      if (size <= old) {
        // New size is smaller or equal, no need to reallocate, just return original pointer
        return ptr;
      }
      // Need to reallocate to a larger size
      auto new_ptr_result = allocate(size);
      if (new_ptr_result.is_error()) {
        return new_ptr_result; // Return error if new allocation fails
      }
      uint64_t *new_ptr = new_ptr_result.value();
      memcpy(new_ptr, ptr, old); // Copy old data
      pool.free(ptr); // Free old pointer
      return new_ptr; // Return new pointer
    }
    return fk::core::Error::NotFound; // Indicate pointer not found in this pool
  };

  // Try reallocating in each pool
  auto result8 = try_move(heap_allocator().alloc8());
  if (!result8.is_error()) {
      return result8;
  }

  auto result16 = try_move(heap_allocator().alloc16());
  if (!result16.is_error()) {
      return result16;
  }

  auto result4096 = try_move(heap_allocator().alloc4096());
  if (!result4096.is_error()) {
      return result4096;
  }

  auto result16384 = try_move(heap_allocator().alloc16384());
  if (!result16384.is_error()) {
      return result16384;
  }

  // If pointer was not found in any of our managed pools, it's an error.
  fk::algorithms::kerror("HEAP MALLOC", "Pointer %p not found in any managed allocator for reallocate.", ptr);
  return fk::core::Error::NotFound;
}

} // namespace containers
} // namespace fk

extern "C" {

void *kcalloc(size_t nmemb, size_t size) {
  if (!nmemb || !size)
    return nullptr;
  size_t total = nmemb * size;
  auto result = fk::containers::allocate(static_cast<size_t>(total));
  if (result.is_error()) {
      // Error already logged by allocate, just return nullptr
      return nullptr;
  }
  // If allocation succeeded, zero it out
  memset(result.value(), 0, total);
  return reinterpret_cast<void*>(result.value());
}

void *kmalloc(size_t size) {
  if (!size)
    return nullptr;
  auto result = fk::containers::allocate(static_cast<size_t>(size));
  if (result.is_error()) {
      // Error already logged by allocate, just return nullptr
      return nullptr;
  }
  return reinterpret_cast<void*>(result.value());
}

void kfree(void *ptr) {
  if (!ptr)
    return;
  fk::containers::free(reinterpret_cast<uint64_t *>(ptr));
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);
  
  // krealloc needs to handle the Result from reallocate
  auto result = fk::containers::reallocate(reinterpret_cast<uint64_t *>(ptr), static_cast<size_t>(size));
  if (result.is_error()) {
      // Log the error and return nullptr
      fk::algorithms::kerror("KREALLOC", "Reallocation failed with error: %d", static_cast<int>(result.error()));
      return nullptr;
  }
  return reinterpret_cast<void*>(result.value());
}

} // extern "C"