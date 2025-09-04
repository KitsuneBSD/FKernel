#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>

class AllocationBitmap {
private:
  uint8_t *m_data{nullptr};
  size_t m_size{0};

  AllocationBitmap(uint8_t *data, size_t size) : m_data(data), m_size(size) {}

public:
  static AllocationBitmap wrap(uint8_t *data, size_t bits) {
    return AllocationBitmap(data, bits);
  }

  bool get(size_t index) const {
    if (index > m_size)
      return false;
    return 0 != (m_data[index >> 3] & (1u << (index & 7)));
  }

  void set(size_t index, bool value) const {
    if (index > m_size)
      return;
    if (value)
      m_data[index >> 3] |= static_cast<uint8_t>(1u << (index & 7));
    else
      m_data[index >> 3] &= static_cast<uint8_t>(~(1u << (index & 7)));
  }
};

template <size_t ChunkSize> class ChunkAllocator {
private:
  uint8_t *m_base{nullptr};
  size_t m_free{capacityInAllocations()};

public:
  void initialize(uint8_t *base) {
    m_base = base;
    m_free = capacityInAllocations();
    memset(m_base, 0, sizeOfAllocationBitmapInBytes());
  }

  static constexpr size_t capacityInAllocations() {
    return 1048576u / ChunkSize; // 1 MiB por pool
  }

  static constexpr size_t capacityInBytes() {
    return capacityInAllocations() * ChunkSize;
  }

  uint8_t *allocate() {
    auto bm = bitmap();
    const size_t cap = capacityInAllocations();
    for (size_t i = 0; i < cap; ++i) {
      if (!bm.get(i)) {
        bm.set(i, true);
        --m_free;
        return pointerToChunk(i);
      }
    }
    return nullptr;
  }

  void free(uint8_t *ptr) {
    auto bm = bitmap();
    auto idx = chunkIndexFromPointer(ptr);
    bm.set(idx, false);
    ++m_free;
  }

  bool isInAllocator(const uint8_t *ptr) const {
    return ptr >= pointerToChunk(0) && ptr < addressAfterThisAllocator();
  }

  size_t chunkIndexFromPointer(const uint8_t *ptr) const {
    return static_cast<size_t>((ptr - pointerToChunk(0)) / ChunkSize);
  }

  uint8_t *pointerToChunk(size_t index) const {
    return m_base + sizeOfAllocationBitmapInBytes() + (index * ChunkSize);
  }

  AllocationBitmap bitmap() const {
    return AllocationBitmap::wrap(m_base, capacityInAllocations());
  }

  static constexpr size_t sizeOfAllocationBitmapInBytes() {
    return (capacityInAllocations() + 7u) / 8u;
  }

  uint8_t *addressAfterThisAllocator() const {
    return m_base + sizeOfAllocationBitmapInBytes() + capacityInBytes();
  }

  size_t chunk_size() const { return ChunkSize; }
};

struct Allocator {
  void initialize();
  void initialize(uint8_t *heap_start, uint8_t *heap_end);
  void initializeIfNeeded();

  ChunkAllocator<8> alloc8;
  ChunkAllocator<16> alloc16;
  ChunkAllocator<4096> alloc4096;
  ChunkAllocator<16384> alloc16384;

  uint8_t *space{nullptr};
  bool initialized{false};
};

#ifdef __cplusplus
extern "C" {
#endif

void *kcalloc(size_t nmemb, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif
