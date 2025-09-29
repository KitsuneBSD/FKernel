#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/bitmap.h>
#include <LibFK/rb_tree.h>

extern "C" uintptr_t __kernel_end;
extern "C" uintptr_t __heap_start;
extern "C" uintptr_t __heap_end;

enum class MemoryType : uint32_t {
  Usable,
  Reserved,
};

struct PhysicalMemoryRange {
  uintptr_t m_start;
  uintptr_t m_end;
  MemoryType m_type;
  bool m_is_on_use;

  bool operator<(const PhysicalMemoryRange &other) const {
    if (m_type != other.m_type)
      return m_type < other.m_type;
    return m_start < other.m_start;
  }

  bool operator>(const PhysicalMemoryRange &other) const {
    if (m_type != other.m_type)
      return m_type > other.m_type;
    return m_start > other.m_start;
  }

  bool operator==(const PhysicalMemoryRange &other) const {
    return m_type == other.m_type && m_start == other.m_start &&
           m_end == other.m_end;
  }
};

class PhysicalMemoryManager {
private:
  rb_tree<PhysicalMemoryRange> m_memory_ranges;
  bool is_initialized = false;

  PhysicalMemoryManager() = default;
  ~PhysicalMemoryManager() = default;

  PhysicalMemoryManager(const PhysicalMemoryManager &) = delete;
  PhysicalMemoryManager &operator=(const PhysicalMemoryManager &) = delete;
  PhysicalMemoryManager(PhysicalMemoryManager &&) = delete;
  PhysicalMemoryManager &operator=(PhysicalMemoryManager &&) = delete;

public:
  static PhysicalMemoryManager &the() {
    static PhysicalMemoryManager instance;
    return instance;
  }

  void initialize(const multiboot2::TagMemoryMap *mmap);
  void *allocate_page();
  void free_page(void *page);
};
