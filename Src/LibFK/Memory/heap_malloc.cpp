#include <LibC/assert.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h> // Assuming this is needed for Result and Error
#include <LibFK/Memory/heap_malloc.h>

namespace fk {
namespace memory {

extern "C" uint64_t __heap_start[];
extern "C" uint64_t __heap_end[];

Allocator s_allocator;

Allocator &heap_allocator() { return s_allocator; }

size_t Allocator::_calculate_required_heap_size() const {
  size_t required = 0;
  required += ChunkAllocator<8>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<8>::capacityInBytes();
  required += ChunkAllocator<16>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<16>::capacityInBytes();
  required += ChunkAllocator<32>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<32>::capacityInBytes();
  required += ChunkAllocator<64>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<64>::capacityInBytes();
  required += ChunkAllocator<128>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<128>::capacityInBytes();
  required += ChunkAllocator<256>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<256>::capacityInBytes();
  required += ChunkAllocator<512>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<512>::capacityInBytes();
  required += ChunkAllocator<1024>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<1024>::capacityInBytes();
  required += ChunkAllocator<2048>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<2048>::capacityInBytes();
  required += ChunkAllocator<4096>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<4096>::capacityInBytes();
  required += ChunkAllocator<8192>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<8192>::capacityInBytes();
  required += ChunkAllocator<16384>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<16384>::capacityInBytes();
  required += ChunkAllocator<32768>::sizeOfAllocationBitmapInBytes() +
              ChunkAllocator<32768>::capacityInBytes();
  return required;
}

void Allocator::initialize() {
  if (heap_allocator().initialized()) {
    fk::algorithms::kerror("HEAP MALLOC", "Allocator is already initialized!");
    return;
  }

  uint64_t *heap_start = __heap_start;
  uint64_t *heap_end = __heap_end;

  ASSERT(heap_start && heap_end && heap_end > heap_start);

  const size_t heap_size = static_cast<size_t>(heap_end - heap_start);

  size_t required = _calculate_required_heap_size();

  ASSERT(heap_size >= required);
  auto space = reinterpret_cast<uint64_t *>(heap_start);

  _initialize_chunk_allocators(space);

  heap_allocator().initialized() = true;
}

void Allocator::_initialize_chunk_allocators(uint64_t* space) {
  heap_allocator().alloc8().initialize(space);
  heap_allocator().alloc16().initialize(
      heap_allocator().alloc8().addressAfterThisAllocator());
  heap_allocator().alloc32().initialize(
      heap_allocator().alloc16().addressAfterThisAllocator());
  heap_allocator().alloc64().initialize(
      heap_allocator().alloc32().addressAfterThisAllocator());
  heap_allocator().alloc128().initialize(
      heap_allocator().alloc64().addressAfterThisAllocator());
  heap_allocator().alloc256().initialize(
      heap_allocator().alloc128().addressAfterThisAllocator());
  heap_allocator().alloc512().initialize(
      heap_allocator().alloc256().addressAfterThisAllocator());
  heap_allocator().alloc1024().initialize(
      heap_allocator().alloc512().addressAfterThisAllocator());
  heap_allocator().alloc2048().initialize(
      heap_allocator().alloc1024().addressAfterThisAllocator());
  heap_allocator().alloc4096().initialize(
      heap_allocator().alloc2048().addressAfterThisAllocator());
  heap_allocator().alloc8192().initialize(
      heap_allocator().alloc4096().addressAfterThisAllocator());
  heap_allocator().alloc16384().initialize(
      heap_allocator().alloc8192().addressAfterThisAllocator());
  heap_allocator().alloc32768().initialize(
      heap_allocator().alloc16384().addressAfterThisAllocator());
}

// Modified allocate function to return Result
fk::core::Result<uint64_t *, fk::core::Error> allocate(size_t size) {
  ASSERT(size > 0);

  if (size <= 8)
    return heap_allocator().alloc8().allocate();
  if (size <= 16)
    return heap_allocator().alloc16().allocate();
  if (size <= 32)
    return heap_allocator().alloc32().allocate();
  if (size <= 64)
    return heap_allocator().alloc64().allocate();
  if (size <= 128)
    return heap_allocator().alloc128().allocate();
  if (size <= 256)
    return heap_allocator().alloc256().allocate();
  if (size <= 512)
    return heap_allocator().alloc512().allocate();
  if (size <= 1024)
    return heap_allocator().alloc1024().allocate();
  if (size <= 2048)
    return heap_allocator().alloc2048().allocate();
  if (size <= 4096)
    return heap_allocator().alloc4096().allocate();
  if (size <= 8192)
    return heap_allocator().alloc8192().allocate();
  if (size <= 16384)
    return heap_allocator().alloc16384().allocate();
  if (size <= 32768)
    return heap_allocator().alloc32768().allocate();

  fk::algorithms::kerror(
      "HEAP MALLOC",
      "Requested size %zu exceeds maximum supported allocation size.", size);
  return fk::core::Error::OutOfMemory;
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
  if (heap_allocator().alloc32().isInAllocator(ptr)) {
    heap_allocator().alloc32().free(ptr);
    return;
  }
  if (heap_allocator().alloc64().isInAllocator(ptr)) {
    heap_allocator().alloc64().free(ptr);
    return;
  }
  if (heap_allocator().alloc128().isInAllocator(ptr)) {
    heap_allocator().alloc128().free(ptr);
    return;
  }
  if (heap_allocator().alloc256().isInAllocator(ptr)) {
    heap_allocator().alloc256().free(ptr);
    return;
  }
  if (heap_allocator().alloc512().isInAllocator(ptr)) {
    heap_allocator().alloc512().free(ptr);
    return;
  }
  if (heap_allocator().alloc1024().isInAllocator(ptr)) {
    heap_allocator().alloc1024().free(ptr);
    return;
  }
  if (heap_allocator().alloc2048().isInAllocator(ptr)) {
    heap_allocator().alloc2048().free(ptr);
    return;
  }
  if (heap_allocator().alloc4096().isInAllocator(ptr)) {
    heap_allocator().alloc4096().free(ptr);
    return;
  }
  if (heap_allocator().alloc8192().isInAllocator(ptr)) {
    heap_allocator().alloc8192().free(ptr);
    return;
  }
  if (heap_allocator().alloc16384().isInAllocator(ptr)) {
    heap_allocator().alloc16384().free(ptr);
    return;
  }
  if (heap_allocator().alloc32768().isInAllocator(ptr)) {
    heap_allocator().alloc32768().free(ptr);
    return;
  }
}

template <typename ChunkAllocatorType>
fk::core::Result<uint64_t *, fk::core::Error>
Allocator::_try_reallocate_in_pool(ChunkAllocatorType& pool, uint64_t* ptr, size_t size) {
  if (pool.isInAllocator(ptr)) {
    size_t old = pool.chunk_size();
    if (size <= old) {
      return ptr; // New size is smaller or equal, return original pointer
    }
    auto new_ptr_result = allocate(size);
    if (new_ptr_result.is_error()) {
      return new_ptr_result; // Return error if new allocation fails
    }
    uint64_t *new_ptr = new_ptr_result.value();
    memcpy(new_ptr, ptr, old);
    pool.free(ptr);
    return new_ptr;
  }
  return fk::core::Error::NotFound; // Indicate pointer not found in this pool
}

// Modified reallocate to handle Result
fk::core::Result<uint64_t *, fk::core::Error> reallocate(uint64_t *ptr,
                                                         size_t size) {
  ASSERT(ptr || size > 0);

  auto result8 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc8(), ptr, size);
  if (result8.is_ok()) return result8;
  auto result16 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc16(), ptr, size);
  if (result16.is_ok()) return result16;
  auto result32 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc32(), ptr, size);
  if (result32.is_ok()) return result32;
  auto result64 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc64(), ptr, size);
  if (result64.is_ok()) return result64;
  auto result128 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc128(), ptr, size);
  if (result128.is_ok()) return result128;
  auto result256 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc256(), ptr, size);
  if (result256.is_ok()) return result256;
  auto result512 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc512(), ptr, size);
  if (result512.is_ok()) return result512;
  auto result1024 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc1024(), ptr, size);
  if (result1024.is_ok()) return result1024;
  auto result2048 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc2048(), ptr, size);
  if (result2048.is_ok()) return result2048;
  auto result4096 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc4096(), ptr, size);
  if (result4096.is_ok()) return result4096;
  auto result8192 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc8192(), ptr, size);
  if (result8192.is_ok()) return result8192;
  auto result16384 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc16384(), ptr, size);
  if (result16384.is_ok()) return result16384;
  auto result32768 = heap_allocator()._try_reallocate_in_pool(heap_allocator().alloc32768(), ptr, size);
  if (result32768.is_ok()) return result32768;

  // If pointer was not found in any of our managed pools, it's an error.
  fk::algorithms::kerror(
      "HEAP MALLOC",
      "Pointer %p not found in any managed allocator for reallocate.", ptr);
  return fk::core::Error::NotFound;
}

} // namespace memory
} // namespace fk

extern "C" {

void *kcalloc(size_t nmemb, size_t size) {
  if (!nmemb || !size)
    return nullptr;
  size_t total = nmemb * size;
  auto result = fk::memory::allocate(static_cast<size_t>(total));
  if (result.is_error()) {
    // Error already logged by allocate, just return nullptr
    return nullptr;
  }
  // If allocation succeeded, zero it out
  memset(result.value(), 0, total);
  return reinterpret_cast<void *>(result.value());
}

void *heap_malloc(size_t size) {
  if (!size)
    return nullptr;
  auto result = fk::memory::allocate(static_cast<size_t>(size));
  if (result.is_error()) {
    // Error already logged by allocate, just return nullptr
    return nullptr;
  }
  return reinterpret_cast<void *>(result.value());
}

void heap_free(void *ptr) {
  if (!ptr)
    return;
  fk::memory::free(reinterpret_cast<uint64_t *>(ptr));
}

void *heap_realloc(void *ptr, size_t size) {
  if (!ptr)
    return heap_malloc(size);

  auto result = fk::memory::reallocate(reinterpret_cast<uint64_t *>(ptr),
                                       static_cast<size_t>(size));
  if (result.is_error()) {
    fk::algorithms::kerror("HEAP MALLOC", "Reallocation failed with error: %d",
                           static_cast<int>(result.error()));
    return nullptr;
  }
  return reinterpret_cast<void *>(result.value());
}

void *kmalloc(size_t size) {
  if (!size)
    return nullptr;
  auto result = fk::memory::allocate(static_cast<size_t>(size));
  if (result.is_error()) {
    // Error already logged by allocate, just return nullptr
    return nullptr;
  }
  return reinterpret_cast<void *>(result.value());
}

void kfree(void *ptr) {
  if (!ptr)
    return;
  fk::memory::free(reinterpret_cast<uint64_t *>(ptr));
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);

  // krealloc needs to handle the Result from reallocate
  auto result = fk::memory::reallocate(reinterpret_cast<uint64_t *>(ptr),
                                       static_cast<size_t>(size));
  if (result.is_error()) {
    // Log the error and return nullptr
    fk::algorithms::kerror("KREALLOC", "Reallocation failed with error: %d",
                           static_cast<int>(result.error()));
    return nullptr;
  }
  return reinterpret_cast<void *>(result.value());
}

} // extern "C"
