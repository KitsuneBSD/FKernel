#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>

#include <LibFK/log.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (is_initialized) {
    kerror("PHYSICAL MEMORY", "Already initialized!");
    return;
  }

  for (const auto &entry : *mmap) {
    uintptr_t start = static_cast<uintptr_t>(entry.base_addr);
    uintptr_t end = start + static_cast<uintptr_t>(entry.length);

    if (end <= start)
      continue;
    if (start < 0x100000)
      start = 0x100000;

    PhysicalMemoryRange range{.m_start = (start + 4095) & ~uintptr_t(4095),
                              .m_end = end & ~uintptr_t(4095),
                              .m_type = multiboot2::is_available(entry.type)
                                            ? MemoryType::Usable
                                            : MemoryType::Reserved};

    if (range.m_start >= range.m_end)
      continue;

    if (!m_memory_ranges.insert(range)) {
      kwarn("PHYSICAL MEMORY", "Failed to insert memory range %p-%p",
            range.m_start, range.m_end);
    }
  }

  klog("PHYSICAL MEMORY", "Initialized Physical Memory Manager");
  is_initialized = true;
}

void *PhysicalMemoryManager::allocate_page() {
  rb_node<PhysicalMemoryRange> *node = m_memory_ranges.root();
  while (node) {
    PhysicalMemoryRange &range = node->value();
    if (range.m_type == MemoryType::Usable && range.size() >= 4096) {
      uintptr_t addr = range.m_start;
      range.m_start += 4096;

      if (range.m_start >= range.m_end) {
        m_memory_ranges.remove(range);
      }

      return reinterpret_cast<void *>(addr);
    }

    if (node->left())
      node = node->left();
    else if (node->right())
      node = node->right();
    else
      break;
  }

  kwarn("PHYSICAL MEMORY", "No usable pages available");
  return nullptr;
}

void PhysicalMemoryManager::free_page(void *page) {
  if (!page) {
    kwarn("PHYSICAL MEMORY", "Attempt to free nullptr");
    return;
  }

  uintptr_t addr = reinterpret_cast<uintptr_t>(page);
  rb_node<PhysicalMemoryRange> *node = nullptr;

  for (rb_node<PhysicalMemoryRange> *n = m_memory_ranges.root(); n;) {
    PhysicalMemoryRange &r = n->value();
    if (r.contains(addr)) {
      node = n;
      break;
    }

    if (addr < r.m_start)
      n = n->left();
    else
      n = n->right();
  }

  if (!node) {
    PhysicalMemoryRange new_range{addr, addr + 4096, MemoryType::Usable};
    if (!m_memory_ranges.insert(new_range)) {
      kwarn("PHYSICAL MEMORY", "Failed to insert freed page %p", addr);
    }
    return;
  }

  PhysicalMemoryRange &range = node->value();

  if (addr >= range.m_start && addr < range.m_end) {
    kwarn("PHYSICAL MEMORY", "Page %p already free (double free?)", addr);
    return;
  }

  if (addr + 4096 == range.m_start)
    range.m_start = addr;
  else if (addr == range.m_end)
    range.m_end = addr + 4096;
  else {
    PhysicalMemoryRange new_range{addr, addr + 4096, MemoryType::Usable};
    if (!m_memory_ranges.insert(new_range)) {
      kwarn("PHYSICAL MEMORY", "Failed to insert freed page %p", addr);
    }
  }
}
