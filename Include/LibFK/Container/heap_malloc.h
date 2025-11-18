#pragma once

#include <LibC/string.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/pair.h> // Include Pair definition
#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>

namespace fk {
namespace containers {

/**
 * @brief Fixed-size chunk allocator using a bitmap.
 *
 * @tparam ChunkSize Size of each chunk in bytes.
 * @tparam PoolSizeInBytes Total size of memory pool in bytes.
 * @tparam T Type for the bitmap storage.
 */
template <size_t ChunkSize, size_t PoolSizeInBytes = 1048576,
          typename T = uint64_t>
class ChunkAllocator {
private:
  static constexpr size_t MaxChunks = PoolSizeInBytes / ChunkSize;

  size_t m_free{MaxChunks};
  Bitmap<T, MaxChunks> m_bitmap;

  const Bitmap<T, MaxChunks> &bitmap() const noexcept { return m_bitmap; }
  Bitmap<T, MaxChunks> &bitmap() noexcept { return m_bitmap; }

  ChunkAllocator(const ChunkAllocator &) = delete;
  ChunkAllocator &operator=(const ChunkAllocator &) = delete;
  ChunkAllocator(ChunkAllocator &&) = delete;
  ChunkAllocator &operator=(ChunkAllocator &&) = delete;

public:
  ChunkAllocator() = default;

  void initialize(uint64_t *space) noexcept {
    m_bitmap.clear_all();
    memset(space, 0, capacityInBytes());
  }

  static constexpr size_t capacityInAllocations() noexcept { return MaxChunks; }
  static constexpr size_t capacityInBytes() noexcept {
    return MaxChunks * ChunkSize;
  }
  static constexpr size_t sizeOfAllocationBitmapInBytes() noexcept {
    return (MaxChunks + 7) / 8;
  }

  fk::core::Result<T*, fk::core::Error> allocate() noexcept {
    for (size_t i = 0; i < MaxChunks; ++i) {
      if (!m_bitmap.get(i)) {
        m_bitmap.set(i, true);
        --m_free;
        return fk::core::Result<T*>(pointerToChunk(i));
      }
    }
    // Allocation failed
    return fk::core::Error::OutOfMemory;
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
    return (reinterpret_cast<const uint8_t *>(ptr) -
            reinterpret_cast<const uint8_t *>(pointerToChunk(0))) /
           ChunkSize;
  }

  T *pointerToChunk(size_t index) noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(m_bitmap.data()) +
                                 sizeOfAllocationBitmapInBytes() +
                                 index * ChunkSize);
  }

  const T *pointerToChunk(size_t index) const noexcept {
    return reinterpret_cast<const T *>(
        reinterpret_cast<const uint8_t *>(m_bitmap.data()) +
        sizeOfAllocationBitmapInBytes() + index * ChunkSize);
  }

  T *addressAfterThisAllocator() noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(m_bitmap.data()) +
                                 sizeOfAllocationBitmapInBytes() +
                                 capacityInBytes());
  }

  const T *addressAfterThisAllocator() const noexcept {
    return reinterpret_cast<const T *>(
        reinterpret_cast<const uint8_t *>(m_bitmap.data()) +
        sizeOfAllocationBitmapInBytes() + capacityInBytes());
  }

  constexpr size_t chunk_size() const noexcept { return ChunkSize; }
};

/**
 * @brief Global allocator with pools of multiple sizes.
 */
struct Allocator {
  void initialize();
  void initialize(uint8_t *heap_start, uint8_t *heap_end);

  // Group ChunkAllocators into a Pair to satisfy the two-instance-variable rule.
  fk::utilities::Pair<fk::utilities::Pair<ChunkAllocator<8>, ChunkAllocator<16>>, fk::utilities::Pair<ChunkAllocator<4096>, ChunkAllocator<16384>>> allocators;

  // Group space and initialized flag into a Pair.
  fk::utilities::Pair<uint8_t *, bool> metadata;

  // Helper methods to access the grouped allocators and metadata
  ChunkAllocator<8>& alloc8() { return allocators.first.first; }
  ChunkAllocator<16>& alloc16() { return allocators.first.second; }
  ChunkAllocator<4096>& alloc4096() { return allocators.second.first; }
  ChunkAllocator<16384>& alloc16384() { return allocators.second.second; }

  uint8_t *&space() { return metadata.first; }
  bool& initialized() { return metadata.second; }
};

} // namespace containers
} // namespace fk

extern "C" {

/// @brief Allocate memory. Intended for global operator new.
void *heap_malloc(size_t size);

/// @brief Free memory. Intended for global operator delete.
void heap_free(void *ptr);

/// @brief Allocate zero-initialized memory.
void *kcalloc(size_t nmemb, size_t size);

/// @brief Allocate memory.
void *kmalloc(size_t size);

/// @brief Free memory.
void kfree(void *ptr);

/// @brief Reallocate memory.
void *krealloc(void *ptr, size_t size);

} // extern "C"