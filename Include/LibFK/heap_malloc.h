#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/bitmap.h>

/**
 * @brief Fixed-size chunk allocator using a bitmap.
 *
 * @tparam ChunkSize
 * Size of each chunk in bytes.
 */

template <size_t ChunkSize, size_t PoolSizeInBytes = 1048576>
class ChunkAllocator {
  using BitmapType = Bitmap<uint8_t, (PoolSizeInBytes / ChunkSize)>;

private:
  uint8_t *m_base{nullptr};
  size_t m_free{capacityInAllocations()};
  BitmapType m_bitmap{};

  const BitmapType &bitmap() const noexcept { return m_bitmap; }
  BitmapType &bitmap() noexcept { return m_bitmap; }

public:
  void initialize(uint8_t *base) noexcept {
    m_base = base;
    m_free = capacityInAllocations();
    m_bitmap.clear();
  }

  static constexpr size_t capacityInAllocations() noexcept {
    return PoolSizeInBytes / ChunkSize;
  }

  static constexpr size_t capacityInBytes() noexcept {
    return capacityInAllocations() * ChunkSize;
  }

  uint8_t *allocate() noexcept {
    auto bm = bitmap();
    for (size_t i = 0; i < capacityInAllocations(); ++i) {
      if (!bm.get(i)) {
        bm.set(i, true);
        --m_free;
        return pointerToChunk(i);
      }
    }
    return nullptr;
  }

  void free(uint8_t *ptr) noexcept {
    auto bm = bitmap();
    size_t idx = chunkIndexFromPointer(ptr);
    bm.set(idx, false);
    ++m_free;
  }

  bool isInAllocator(const uint8_t *ptr) const noexcept {
    return ptr >= pointerToChunk(0) && ptr < addressAfterThisAllocator();
  }

  size_t chunkIndexFromPointer(const uint8_t *ptr) const noexcept {
    return static_cast<size_t>((ptr - pointerToChunk(0)) / ChunkSize);
  }

  uint8_t *pointerToChunk(size_t index) const noexcept {
    return m_base + BitmapType::sizeInBytes() + index * ChunkSize;
  }

  uint8_t *addressAfterThisAllocator() const noexcept {
    return m_base + BitmapType::sizeInBytes() + capacityInBytes();
  }

  static constexpr size_t sizeOfAllocationBitmapInBytes() noexcept {
    return BitmapType::sizeInBytes();
  }

  constexpr size_t chunk_size() const noexcept { return ChunkSize; }
};

/**
 * @brief Global allocator with pools of multiple sizes.
 */
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

/// @brief Allocate zero-initialized memory.
void *kcalloc(size_t nmemb, size_t size);

/// @brief Allocate memory.
void *kmalloc(size_t size);

/// @brief Free memory.
void kfree(void *ptr);

/// @brief Reallocate memory.
void *krealloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif
