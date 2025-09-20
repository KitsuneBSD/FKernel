#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>

/**
 * @brief Bitmap for tracking allocation status of chunks.
 *
 * Each bit represents a chunk: 1 = allocated, 0 = free.
 */
class AllocationBitmap {
private:
  uint8_t *m_data{nullptr}; ///< Pointer to raw bitmap storage.
  size_t m_size{0};         ///< Number of bits tracked.

  AllocationBitmap(uint8_t *data, size_t size) : m_data(data), m_size(size) {}

public:
  /**
   * @brief Create a bitmap view from raw memory.
   * @param data Pointer to storage.
   * @param bits Number of bits to manage.
   * @return AllocationBitmap instance.
   */
  static AllocationBitmap wrap(uint8_t *data, size_t bits) {
    return AllocationBitmap(data, bits);
  }

  /**
   * @brief Check if a bit is set.
   * @param index Bit index.
   * @return True if set, false otherwise.
   */
  bool get(size_t index) const noexcept {
    if (index >= m_size)
      return false;
    return 0 != (m_data[index >> 3] & (1u << (index & 7)));
  }

  /**
   * @brief Set or clear a bit.
   * @param index Bit index.
   * @param value True = set, false = clear.
   */
  void set(size_t index, bool value) noexcept {
    if (index >= m_size)
      return;
    if (value)
      m_data[index >> 3] |= static_cast<uint8_t>(1u << (index & 7));
    else
      m_data[index >> 3] &= static_cast<uint8_t>(~(1u << (index & 7)));
  }
};

/**
 * @brief Fixed-size chunk allocator using a bitmap.
 *
 * @tparam ChunkSize Size of each chunk in bytes.
 */
template <size_t ChunkSize> class ChunkAllocator {
private:
  uint8_t *m_base{nullptr}; ///< Pointer to memory pool base.
  size_t m_free{capacityInAllocations()};

public:
  /// @brief Initialize allocator with memory base.
  void initialize(uint8_t *base) noexcept {
    m_base = base;
    m_free = capacityInAllocations();
    memset(m_base, 0, sizeOfAllocationBitmapInBytes());
  }

  /// @return Maximum number of chunks available.
  static constexpr size_t capacityInAllocations() noexcept {
    return 1048576u / ChunkSize; // 1 MiB pool
  }

  /// @return Total capacity in bytes.
  static constexpr size_t capacityInBytes() noexcept {
    return capacityInAllocations() * ChunkSize;
  }

  /**
   * @brief Allocate one chunk.
   * @return Pointer to chunk or nullptr if full.
   */
  uint8_t *allocate() noexcept {
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

  /**
   * @brief Free a previously allocated chunk.
   * @param ptr Pointer to chunk.
   */
  void free(uint8_t *ptr) noexcept {
    auto bm = bitmap();
    auto idx = chunkIndexFromPointer(ptr);
    bm.set(idx, false);
    ++m_free;
  }

  /// @return True if pointer belongs to this allocator.
  bool isInAllocator(const uint8_t *ptr) const noexcept {
    return ptr >= pointerToChunk(0) && ptr < addressAfterThisAllocator();
  }

  /// @return Chunk index corresponding to pointer.
  size_t chunkIndexFromPointer(const uint8_t *ptr) const noexcept {
    return static_cast<size_t>((ptr - pointerToChunk(0)) / ChunkSize);
  }

  /// @return Pointer to chunk by index.
  uint8_t *pointerToChunk(size_t index) const noexcept {
    return m_base + sizeOfAllocationBitmapInBytes() + (index * ChunkSize);
  }

  /// @return Bitmap view for this allocator.
  AllocationBitmap bitmap() const noexcept {
    return AllocationBitmap::wrap(m_base, capacityInAllocations());
  }

  /// @return Size of bitmap in bytes.
  static constexpr size_t sizeOfAllocationBitmapInBytes() noexcept {
    return (capacityInAllocations() + 7u) / 8u;
  }

  /// @return Address immediately after this allocator.
  uint8_t *addressAfterThisAllocator() const noexcept {
    return m_base + sizeOfAllocationBitmapInBytes() + capacityInBytes();
  }

  /// @return Size of each chunk.
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
