#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/log.h>
#include <LibFK/rb_tree.h>
#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

extern "C" uintptr_t __kernel_start;
extern "C" uintptr_t __kernel_end;
extern "C" uintptr_t __heap_start;
extern "C" uintptr_t __heap_end;

#include <Kernel/MemoryManager/MemoryRange.h>

class PhysicalMemoryManager {
  friend class VirtualMemoryManager;
private:
  rb_tree<PhysicalMemoryRange, 65536> m_memory_ranges;
  bool is_initialized = false;

  PhysicalMemoryManager() = default;
  ~PhysicalMemoryManager() = default;

  PhysicalMemoryManager(const PhysicalMemoryManager &) = delete;
  PhysicalMemoryManager &operator=(const PhysicalMemoryManager &) = delete;
  PhysicalMemoryManager(PhysicalMemoryManager &&) = delete;
  PhysicalMemoryManager &operator=(PhysicalMemoryManager &&) = delete;

  int alloc_from_node(rb_node<PhysicalMemoryRange> *node, size_t count,
                      uintptr_t addr_hint);

public:
  static PhysicalMemoryManager &the() {
    static PhysicalMemoryManager instance;
    return instance;
  }

  void initialize(const multiboot2::TagMemoryMap *mmap);
  void *alloc_physical_page(size_t count, uintptr_t addr_hint = 0);
  void free_physical_page(void *page);
};
