#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Memory/MemoryRange.h>

#include <Kernel/Memory/PhysicalMemoryManager.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (is_initialized) {
    fk::algorithms::kerror("PHYSICAL MEMORY", "Already initialized!");
    return;
  }

  for (const auto &entry : *mmap) {
    auto entry_start = entry.base_addr;
    auto entry_end = entry.base_addr + entry.length;

    if (entry_start < 2 * fk::types::MiB) {
      entry_start = 2 * fk::types::MiB;
    }

    if (entry_end <= entry_start) {
      continue;
    }

    auto current = m_memory_ranges.root();
    while (current) {
      auto &r = current->value();
      if (entry_end <= r.m_start) {
        current = current->left();
      } else if (entry_start >= r.m_end) {
        current = current->right();
      } else {
        if (entry_start < r.m_start)
          entry_end = r.m_start;
        else if (entry_end > r.m_end)
          entry_start = r.m_end;

        if (entry_end <= entry_start) {
          fk::algorithms::kwarn("PHYSICAL MEMORY",
                                "Skipped overlapped range [%p - %p]",
                                entry_start, entry_end);
          break;
        }
        current = m_memory_ranges.root();
      }
    }

    MemoryType type = multiboot2::is_available(entry.type)
                          ? MemoryType::Usable
                          : MemoryType::Reserved;
    PhysicalMemoryRange range{
        .m_start = entry_start,
        .m_end = entry_end,
        .m_type = type,
        .m_bitmap = fk::containers::Bitmap<uint64_t, 65535>(),
        .m_page_count = 0,
    };

    range.initialize();

    if (m_memory_ranges.insert(range).is_error()) {
      fk::algorithms::kwarn("PHYSICAL MEMORY",
                            "We can't insert the new memory range [%p - %p]",
                            entry_start, entry_end);
    }

    fk::algorithms::kdebug(
        "PHYSICAL MEMORY", "Insert a new range of memory [ %p - %p | %s ]",
        entry_start, entry_end,
        multiboot2::is_available(entry.type) ? "usable" : "reserved");
  }

  is_initialized = true;
  fk::algorithms::klog("PHYSICAL MEMORY",
                       "Physical memory manager initialized");
}

int PhysicalMemoryManager::alloc_from_node(
    fk::containers::rb_node<PhysicalMemoryRange> *node, size_t count,
    uintptr_t addr_hint) {
  if (!node)
    return -1;

  int index = alloc_from_node(node->left(), count, addr_hint);
  if (index >= 0)
    return index;

  // TODO: If we work on continuous memory we can make allocation of more pages
  // instead one page per range
  if (node->value().m_type == MemoryType::Usable) {
    index = node->value().alloc_page(count, addr_hint);
    if (index >= 0) {
      uintptr_t addr = node->value().m_start + index * PAGE_SIZE;
      // kdebug("PHYSICAL MEMORY", "Allocated %zu page(s) at 0x%lx", count,
      // addr);
      return addr;
    }
  }

  return alloc_from_node(node->right(), count, addr_hint);
}

void *PhysicalMemoryManager::alloc_physical_page(size_t count,
                                                 uintptr_t addr_hint) {
  if (!is_initialized) {
    return nullptr;
  }

  if (count == 0) {
    fk::algorithms::kwarn("PHYSICAL MEMORY", "Cannot allocate zero pages");
    return nullptr;
  }

  uintptr_t addr = alloc_from_node(m_memory_ranges.root(), count, addr_hint);
  if (addr == static_cast<uintptr_t>(-1)) {
    fk::algorithms::kwarn("PHYSICAL MEMORY",
                          "No more physical memory available");
    return nullptr;
  }

  return reinterpret_cast<void *>(addr);
}

void PhysicalMemoryManager::free_physical_page(void *page) {
  if (!is_initialized) {
    return;
  }

  uintptr_t addr = reinterpret_cast<uintptr_t>(page);

  fk::containers::rb_node<PhysicalMemoryRange> *current =
      m_memory_ranges.root();
  while (current) {
    auto &range = current->value();
    if (addr < range.m_start) {
      current = current->left();
    } else if (addr >= range.m_end) {
      current = current->right();
    } else {
      range.free_page(addr);
      return;
    }
  }

  fk::algorithms::kwarn("PHYSICAL MEMORY",
                        "The address %p does not belong to any memory range",
                        addr);
}

uintptr_t PhysicalMemoryManager::virt_to_phys(uintptr_t addr) {
  fk::containers::rb_node<PhysicalMemoryRange> *current =
      m_memory_ranges.root();
  while (current) {
    auto &range = current->value();
    if (addr < range.m_start) {
      current = current->left();
    } else if (addr >= range.m_end) {
      current = current->right();
    } else {
      return addr - range.m_start + range.m_start;
    }
  }

  fk::algorithms::kwarn("PHYSICAL MEMORY",
                        "The address %p does not belong to any memory range",
                        addr);
  return 0;
}
