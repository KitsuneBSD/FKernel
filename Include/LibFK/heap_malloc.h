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

template <size_t ChunkSize, size_t PoolSizeInBytes = 1048576, typename T = uint64_t>
class ChunkAllocator {
private:
  size_t m_free{capacityInAllocations()};
  Bitmap<T> m_bitmap;

  const Bitmap<T> &bitmap() const noexcept { return m_bitmap; }
  Bitmap<T> &bitmap() noexcept { return m_bitmap; }

  ChunkAllocator(const ChunkAllocator &) = delete;
  ChunkAllocator &operator=(const ChunkAllocator &) = delete;
  ChunkAllocator(ChunkAllocator &&) = delete;
  ChunkAllocator &operator=(ChunkAllocator &&) = delete;
public:
  ChunkAllocator() = default; 

  void initialize(T *space) noexcept {
    m_bitmap = Bitmap<T>(sizeOfAllocationBitmapInBytes() * 8);
    memset(space, 0, sizeOfAllocationBitmapInBytes() + capacityInBytes());
  }

  static constexpr size_t capacityInAllocations() noexcept {
    return PoolSizeInBytes / ChunkSize;
  }

  static constexpr size_t capacityInBytes() noexcept {
    return capacityInAllocations() * ChunkSize;
  }

  static constexpr size_t sizeOfAllocationBitmapInBytes() noexcept {
    return (capacityInAllocations() + (sizeof(T) -1)) / sizeof(T);
  }

  T *allocate() noexcept {
    for (size_t i = 0; i < capacityInAllocations(); ++i) {
      if (!m_bitmap.get(i)) {
        m_bitmap.set(i, true);
        --m_free;
        return pointerToChunk(i);
      }
    }
    return nullptr;
  }

  void free(T *ptr) noexcept {
    size_t idx = chunkIndexFromPointer(ptr);
    m_bitmap.set(idx, false);
    ++m_free;
  }

  bool isInAllocator(const T *ptr) const noexcept {
    return ptr >= pointerToChunk(0) && ptr < addressAfterThisAllocator();
  }

  size_t chunkIndexFromPointer(const T *ptr) const noexcept {
    return static_cast<size_t>((ptr - pointerToChunk(0)) / ChunkSize);
  }

  T *pointerToChunk(size_t index) noexcept {
    return reinterpret_cast<T*>(
        reinterpret_cast<uint64_t*>(m_bitmap.data()) + sizeOfAllocationBitmapInBytes() + (index * ChunkSize)
    );
  }

  const T *pointerToChunk(size_t index) const noexcept {
    return reinterpret_cast<const T*>(
        reinterpret_cast<const uint8_t*>(m_bitmap.data())
        + sizeOfAllocationBitmapInBytes()
        + index * ChunkSize
    );
  }

  T *addressAfterThisAllocator() noexcept {
    return reinterpret_cast<T*>(
        reinterpret_cast<uint64_t*>(m_bitmap.data()) + sizeOfAllocationBitmapInBytes() + capacityInBytes()
    );
  }

  const T *addressAfterThisAllocator() const noexcept {
    return reinterpret_cast<const T*>(
        reinterpret_cast<const uint8_t*>(m_bitmap.data())
        + sizeOfAllocationBitmapInBytes()
        + capacityInBytes()
    );
  }

  constexpr size_t chunk_size() const noexcept { return ChunkSize; }
};

/** 
 * @brief Global allocator with pools of multiple sizes.
 */
struct Allocator {
  void initialize();
  void initialize(uint8_t *heap_start, uint8_t *heap_end);

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
