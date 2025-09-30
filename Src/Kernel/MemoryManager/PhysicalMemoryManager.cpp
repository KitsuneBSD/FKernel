#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>

#include <LibFK/log.h>
#include <LibFK/types.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (is_initialized) {
    kerror("PHYSICAL MEMORY", "Already initialized!");
    return;
  }

  for (const auto &entry : *mmap) {
    auto entry_start = entry.base_addr;
    auto entry_end = entry.base_addr + entry.length;

    if (entry_start < 1 * MiB) {
      kwarn("PHYSICAL MEMORY", "We can't map the entry low than 1 MiB: %p",
            entry_start);
      entry_start = 1 * MiB;
      continue;
    }

    if (entry_end <= entry_start) {
      kwarn("PHYSICAL MEMORY", "Invalid memory range [%p - %p]", entry_start,
            entry_end);
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
          kwarn("PHYSICAL MEMORY", "Skipped overlapped range [%p - %p]",
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
    };

    if (!m_memory_ranges.insert(range)) {
      kwarn("PHYSICAL MEMORY", "We can't insert the new memory range [%p - %p]",
            entry_start, entry_end);
    }


    klog("PHYSICAL MEMORY", "Insert a new range of memory [ %p - %p | %s ]",
         entry_start, entry_end, multiboot2::is_available(entry.type) ? "usable" : "reserved");
  }

  is_initialized = true;
  klog("PHYSICAL MEMORY", "Manager initialized with sucess");
}
