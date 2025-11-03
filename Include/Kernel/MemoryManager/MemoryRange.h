#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Types/types.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

/**
 * @brief Physical memory type
 */
enum class MemoryType : uint32_t {
  Usable,  ///< Memory available for allocation
  Reserved ///< Memory reserved, cannot be used
};

/**
 * @brief Represents a contiguous range of physical memory
 *
 * Manages allocation and freeing of pages within the range using a bitmap.
 */
struct PhysicalMemoryRange {
  uintptr_t m_start; ///< Start address of the memory range
  uintptr_t m_end;   ///< End address (exclusive)
  MemoryType m_type; ///< Type of memory (usable or reserved)

  Bitmap<uint64_t, 65535> m_bitmap; ///< Bitmap tracking allocated pages
  size_t m_page_count;              ///< Total number of pages in this range

  /**
   * @brief Initialize the memory range and bitmap
   *
   * Calculates the number of pages and clears the bitmap.
   */
  void initialize() {
    m_page_count = (m_end - m_start) / PAGE_SIZE;
    m_bitmap.resize(m_page_count);
    m_bitmap.clear_all();
  }

  /**
   * @brief Allocate one or more pages from this range
   * @param count Number of pages to allocate (default 1)
   * @param addr_hint Optional starting address hint
   * @return Index of the first allocated page on success, -1 on failure
   */
  int alloc_page(size_t count = 1, uintptr_t addr_hint = 0) {
    if (m_type != MemoryType::Usable || count == 0 || count > m_page_count) {
      kwarn("PHYSICAL MEMORY RANGE",
            "Cannot allocate %zu pages in range [%p - %p]", count, m_start,
            m_end);
      return -1;
    }

    size_t start_index = 0;
    if (addr_hint) {
      if (addr_hint >= m_start && addr_hint + count * PAGE_SIZE <= m_end) {
        start_index = (addr_hint - m_start) / PAGE_SIZE;
      } else {
        kwarn("PHYSICAL MEMORY RANGE", "Address hint %p out of range [%p - %p]",
              addr_hint, m_start, m_end);
        addr_hint = 0;
      }
    }

    for (size_t i = start_index; i <= m_page_count - count; i++) {
      bool found = true;
      for (size_t j = 0; j < count; ++j) {
        if (m_bitmap.get(i + j)) {
          found = false;
          break;
        }
      }

      if (found) {
        for (size_t j = 0; j < count; ++j) {
          m_bitmap.set(i + j, true);
        }
        return static_cast<int>(i);
      }

      if (addr_hint) {
        addr_hint = 0;
        i = static_cast<size_t>(-1);
      }
    }

    kwarn("PHYSICAL MEMORY RANGE",
          "No available pages for %zu pages in range [%p - %p]", count, m_start,
          m_end);
    return -1;
  }

  /**
   * @brief Free one or more previously allocated pages
   * @param addr Starting address of pages to free
   * @param count Number of pages to free (default 1)
   */
  void free_page(uintptr_t addr, size_t count = 1) {
    if (m_type != MemoryType::Usable || count == 0) {
      kwarn("PHYSICAL MEMORY RANGE",
            "Cannot free pages in this range [%p - %p]", m_start, m_end);
      return;
    }

    if (addr < m_start || addr + count * PAGE_SIZE > m_end) {
      kwarn("PHYSICAL MEMORY RANGE", "Address %p out of range [%p - %p]", addr,
            m_start, m_end);
      return;
    }

    size_t index = (addr - m_start) / PAGE_SIZE;

    for (size_t i = 0; i < count; ++i) {
      if (!m_bitmap.get(index + i)) {
        kwarn("PHYSICAL MEMORY RANGE", "Page at 0x%lx already free",
              addr + i * PAGE_SIZE);
      } else {
        m_bitmap.clear(index + i);
      }
    }
  }

  /**
   * @brief Compare ranges by starting address
   */
  bool operator<(const PhysicalMemoryRange &other) const {
    return m_start < other.m_start;
  }

  /**
   * @brief Check if two ranges are equal
   */
  bool operator==(const PhysicalMemoryRange &other) const {
    return m_start == other.m_start && m_end == other.m_end;
  }
};
