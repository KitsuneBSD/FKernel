#include "Kernel/MemoryManager/Pages/PageFlags.h"
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

extern "C" void write_on_cr3(void *pml4);

void map_ranges_iterative(rb_node<PhysicalMemoryRange> *root) {
  constexpr int MAX_STACK = 64;
  rb_node<PhysicalMemoryRange> *stack[MAX_STACK];
  int sp = 0;
  rb_node<PhysicalMemoryRange> *current = root;

  while (current || sp > 0) {
    while (current) {
      if (sp >= MAX_STACK) {
        return;
      }
      stack[sp++] = current;
      current = current->left();
    }

    current = stack[--sp];

    auto &range = current->value();
    /**
    kprintf("Mapping physical memory range [ %p - %p]\n", range.m_start,
            range.m_end);
**/
    VirtualMemoryManager::the().map_range(
        range.m_start, range.m_start, range.m_end - range.m_start,
        PageFlags::Present | PageFlags::Writable | PageFlags::HugePage);

    current = current->right();
  }
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys,
                                    uint64_t flags,
                                    [[maybe_unused]] uint64_t page_size) {
  if (PhysicalMemoryManager::the().virt_to_phys(virt) == phys) {
    return;
  }

  uint64_t pml4_index = (virt >> 39) & 0x1FF;
  uint64_t pdpt_index = (virt >> 30) & 0x1FF;
  uint64_t pd_index = (virt >> 21) & 0x1FF;
  uint64_t pt_index = (virt >> 12) & 0x1FF;

  uint64_t *pdpt;
  if (!(m_pml4[pml4_index] & PageFlags::Present)) {
    pdpt = alloc_table();
    m_pml4[pml4_index] = reinterpret_cast<uintptr_t>(pdpt) |
                         PageFlags::Present | PageFlags::Writable;
  } else {
    pdpt = reinterpret_cast<uint64_t *>(m_pml4[pml4_index] & PAGE_MASK);
  }

  uint64_t *pd;
  if (!(pdpt[pdpt_index] & PageFlags::Present)) {
    pd = alloc_table();
    pdpt[pdpt_index] = reinterpret_cast<uintptr_t>(pd) | PageFlags::Present |
                       PageFlags::Writable;
  } else {
    pd = reinterpret_cast<uint64_t *>(pdpt[pdpt_index] & PAGE_MASK);
  }

  uint64_t *pt;
  if (!(pd[pd_index] & PageFlags::Present)) {
    pt = alloc_table();
    pd[pd_index] = reinterpret_cast<uintptr_t>(pt) | PageFlags::Present |
                   PageFlags::Writable;
  } else {
    pt = reinterpret_cast<uint64_t *>(pd[pd_index] & PAGE_MASK);
  }

  pt[pt_index] = phys | (flags & ~PAGE_MASK);
  asm volatile("invlpg (%0)" ::"r"((void *)virt) : "memory");

  kdebug("VIRTUAL MEMORY", "Mapped page V:0x%lx -> P:0x%lx (flags 0x%lx)", virt,
         phys, flags);
}

void VirtualMemoryManager::map_range(uintptr_t virt_start, uintptr_t phys_start,
                                     size_t size, uint64_t flags) {
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    map_page(virt_start + offset, phys_start + offset, flags, PAGE_SIZE);
  }
}

uint64_t *VirtualMemoryManager::alloc_table() {
  void *page = PhysicalMemoryManager::the().alloc_physical_page(PAGE_SIZE);
  if (!page) {
    kwarn("VIRTUAL MEMORY", "Failed to allocate page for table.");
    return nullptr;
  }

  uint64_t *table = reinterpret_cast<uint64_t *>(page);
  memset(table, 0, PAGE_SIZE);
  return table;
}

void VirtualMemoryManager::initialize() {
  if (m_is_initialized) {
    kwarn("VIRTUAL MEMORY", "Already initialized.");
    return;
  }

  m_pml4 = alloc_table();
  uint64_t *pdpt = alloc_table();
  if (!m_pml4 || !pdpt) {
    kerror("VIRTUAL MEMORY", "Failed to allocate PML4 or PDPT.");
    return;
  }

  m_pml4[0] = reinterpret_cast<uintptr_t>(pdpt) | PageFlags::Present |
              PageFlags::Writable;

  for (int i = 0; i < 4; i++) {
    uint64_t *pd = alloc_table();
    if (!pd) {
      kerror("VIRTUAL MEMORY", "Failed to allocate Page Directory.");
      return;
    }

    pdpt[i] = reinterpret_cast<uintptr_t>(pd) | PageFlags::Present |
              PageFlags::Writable;

    for (int j = 0; j < 512; j++) {
      pd[j] = (i * 512 + j) * PAGE_SIZE_2M;
      pd[j] |= PageFlags::Present | PageFlags::Writable | PageFlags::HugePage;
    }
  }

  write_on_cr3(reinterpret_cast<void *>(m_pml4));

  map_ranges_iterative(PhysicalMemoryManager::the().m_memory_ranges.root());

  klog("VIRTUAL MEMORY", "Virtual Memory Manager initialized");
  m_is_initialized = true;
}
