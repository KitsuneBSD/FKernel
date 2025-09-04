#include <LibFK/heap_malloc.h>
#include <stdio.h>

extern "C" uint8_t __heap_start[];
extern "C" uint8_t __heap_end[];

static Allocator s_allocator;

void Allocator::initialize() {
  uint8_t *heap_start = __heap_start;
  uint8_t *heap_end = __heap_end;
  if (!heap_start || heap_end <= heap_start) {
    return;
  }

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

  if (heap_size < required) {
    printf("The heap has %zu, and is required %zu. Both has diference of %zu\n",
           heap_size, required, required - heap_size);
    return;
  }

  space = heap_start;

  alloc8.initialize(space);
  alloc16.initialize(alloc8.addressAfterThisAllocator());
  alloc4096.initialize(alloc16.addressAfterThisAllocator());
  alloc16384.initialize(alloc4096.addressAfterThisAllocator());
  initialized = true;
}

void Allocator::initialize(uint8_t *heap_start, uint8_t *heap_end) {
  if (!heap_start || heap_end <= heap_start)
    return;

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

  if (heap_size < required)
    return;

  space = heap_start;
  alloc8.initialize(space);
  alloc16.initialize(alloc8.addressAfterThisAllocator());
  alloc4096.initialize(alloc16.addressAfterThisAllocator());
  alloc16384.initialize(alloc4096.addressAfterThisAllocator());
  initialized = true;
}

void Allocator::initializeIfNeeded() {
  if (initialized)
    return;
  initialize();
}

void initialize() { s_allocator.initializeIfNeeded(); }

uint8_t *allocate(size_t size) {
  if (!size)
    return nullptr;
  s_allocator.initializeIfNeeded();
  if (!s_allocator.initialized)
    return nullptr;

  if (size <= 8)
    return s_allocator.alloc8.allocate();
  if (size <= 16)
    return s_allocator.alloc16.allocate();
  if (size <= 4096)
    return s_allocator.alloc4096.allocate();
  if (size <= 16384)
    return s_allocator.alloc16384.allocate();
  return nullptr;
}

uint8_t *allocateZeroed(size_t size) {
  uint8_t *p = allocate(size);
  if (!p)
    return nullptr;
  memset(p, 0, size);
  return p;
}

void free(uint8_t *ptr) {
  if (!ptr)
    return;
  s_allocator.initializeIfNeeded();
  if (!s_allocator.initialized)
    return;

  if (s_allocator.alloc8.isInAllocator(ptr)) {
    s_allocator.alloc8.free(ptr);
    return;
  }
  if (s_allocator.alloc16.isInAllocator(ptr)) {
    s_allocator.alloc16.free(ptr);
    return;
  }
  if (s_allocator.alloc4096.isInAllocator(ptr)) {
    s_allocator.alloc4096.free(ptr);
    return;
  }
  if (s_allocator.alloc16384.isInAllocator(ptr)) {
    s_allocator.alloc16384.free(ptr);
    return;
  }
}

uint8_t *reallocate(uint8_t *ptr, size_t size) {
  if (!ptr)
    return allocate(size);
  if (size == 0) {
    free(ptr);
    return nullptr;
  }

  s_allocator.initializeIfNeeded();
  if (!s_allocator.initialized)
    return nullptr;

  auto try_move = [&](auto &pool) -> uint8_t * {
    if (pool.isInAllocator(ptr)) {
      size_t old = pool.chunk_size();
      if (size <= old)
        return ptr;
      uint8_t *newptr = allocate(size);
      if (!newptr)
        return nullptr;
      memcpy(newptr, ptr, old);
      pool.free(ptr);
      return newptr;
    }
    return nullptr;
  };

  if (auto *p = try_move(s_allocator.alloc8))
    return p;
  if (auto *p = try_move(s_allocator.alloc16))
    return p;
  if (auto *p = try_move(s_allocator.alloc4096))
    return p;
  if (auto *p = try_move(s_allocator.alloc16384))
    return p;

  return nullptr;
}

void *kcalloc(size_t nmemb, size_t size) {
  if (!nmemb || !size)
    return nullptr;
  size_t total = nmemb * size;
  return reinterpret_cast<void *>(allocateZeroed(static_cast<size_t>(total)));
}

void *kmalloc(size_t size) {
  if (!size)
    return nullptr;
  return reinterpret_cast<void *>(allocate(static_cast<size_t>(size)));
}

void kfree(void *ptr) {
  if (!ptr)
    return;
  free(reinterpret_cast<uint8_t *>(ptr));
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);
  return reinterpret_cast<void *>(
      reallocate(reinterpret_cast<uint8_t *>(ptr), static_cast<size_t>(size)));
}
