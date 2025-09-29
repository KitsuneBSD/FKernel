#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/limits.h>
#include <LibFK/log.h>
#include <LibFK/new.h>
#include <LibFK/rb_tree.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (is_initialized) {
    kerror("PHYSICAL MEMORY", "Already initialized!");
    return;
  }

  for (const auto &entry : *mmap) {
    uintptr_t entry_start = static_cast<uintptr_t>(entry.base_addr);
    uintptr_t entry_end = entry_start + static_cast<uintptr_t>(entry.length);

    if (entry_start <= 1 * 1024 * 1024) {
      kwarn("PHYSICAL MEMORY", "Unusable memory %p need be ignored",
            entry_start);
      continue;
    }

    if (entry_end <= entry_start) {
      kwarn("PHYSICAL MEMORY",
            "Entry end %p never can be smaller than entry start %p", entry_end,
            entry_start);
      continue;
    }

    if (multiboot2::is_available(entry.type)) {
      uintptr_t aligned_start =
          (entry_start + 4095) & ~static_cast<uintptr_t>(4095);

      for (uintptr_t addr = aligned_start; addr + 4096 <= entry_end;
           addr += 4096) {
        PhysicalMemoryRange page{
            .m_start = addr,
            .m_end = addr + 4096,
            .m_type = MemoryType::Usable,
            .m_is_on_use = false,
        };
        m_memory_ranges.insert(page);
      }
    } else {
      PhysicalMemoryRange reserved{
          .m_start = entry_start,
          .m_end = entry_end,
          .m_type = MemoryType::Reserved,
          .m_is_on_use = true,
      };
      m_memory_ranges.insert(reserved);
    }
  }

  klog("PHYSICAL MEMORY MANAGER", "Initialized Physical Memory Manager...");
  is_initialized = true;
}
