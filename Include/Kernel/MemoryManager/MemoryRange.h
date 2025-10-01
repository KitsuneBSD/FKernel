#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#include <LibFK/bitmap.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

enum class MemoryType : uint32_t {
  Usable,
  Reserved,
};

struct PhysicalMemoryRange {
  uintptr_t m_start;
  uintptr_t m_end;
  MemoryType m_type;

  Bitmap<uint64_t, 65535> m_bitmap;
  size_t m_page_count;

  void initialize() {
    m_page_count = (m_end - m_start) / PAGE_SIZE;
    m_bitmap.resize(m_page_count);
    m_bitmap.clear_all();
  }

  int alloc_page(size_t count = 1, uintptr_t addr_hint = 0) {
    if (m_type != MemoryType::Usable || count == 0 || count > m_page_count) {
      return -1;
    }

    size_t start_index = 0;
    if (addr_hint) {
      if (addr_hint >= m_start && addr_hint * count + PAGE_SIZE <= m_end) {
        start_index = (addr_hint - m_start) / PAGE_SIZE;
      } else {
        kwarn("PHYSICAL MEMORY RANGE",
              "The address hint %p is out of range [%p - %p]", addr_hint,
              m_start, m_end);
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

    return -1;
  }

  void free_page(uintptr_t addr, size_t count = 1) {
    if (m_type != MemoryType::Usable || count == 0) {
      kwarn("PHYSICAL MEMORY RANGE",
            "We can't free pages in this range [%p - %p]", m_start, m_end);
      return;
    }

    if (addr < m_start || addr + count * PAGE_SIZE > m_end) {
      kwarn("PHYSICAL MEMORY RANGE", "The address %p is out of range [%p - %p]",
            addr, m_start, m_end);
      return;
    }

    size_t index = (addr - m_start) / PAGE_SIZE;

    for (size_t i = 0; i < count; ++i) {
      if (!m_bitmap.get(index + i)) {
        kwarn("PHYSICAL MEMORY RANGE", "The page at address %p is already free",
              addr + i * PAGE_SIZE);
      } else {
        m_bitmap.clear(index + i);
      }
    }
  }

  bool operator<(const PhysicalMemoryRange &other) const {
    return m_start < other.m_start;
  }
  bool operator==(const PhysicalMemoryRange &other) const {
    return m_start == other.m_start && m_end == other.m_end;
  }
};
