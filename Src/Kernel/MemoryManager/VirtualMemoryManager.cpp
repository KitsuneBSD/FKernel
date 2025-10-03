#include "Kernel/MemoryManager/Pages/PageFlags.h"
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

void map_ranges_iterative(rb_node<PhysicalMemoryRange> *root) {
  rb_node<PhysicalMemoryRange> *stack[64];
  int sp = 0;
  rb_node<PhysicalMemoryRange> *current = root;

  while (current || sp > 0) {
    while (current) {
      stack[sp++] = current;
      kprintf("Stacking %p ...\n", current);
      current = current->left();
    }

    current = stack[--sp];

    auto &range = current->value();
    kprintf("Map range [ %p - %p ]\n", range.m_start, range.m_end);
    VirtualMemoryManager::the().map_range(
        range.m_start, range.m_start, range.m_end - range.m_start,
        PageFlags::Present | PageFlags::Writable);

    current = current->right();
  }
}

uint64_t *VirtualMemoryManager::alloc_table() {
  void *page = PhysicalMemoryManager::the().alloc_physical_page(1);
  if (!page) {
    kerror("VIRTUAL MEMORY", "Failed to allocate page table");
    return nullptr;
  }
  uint64_t *table = reinterpret_cast<uint64_t *>(page);
  memset(table, 0, 4096);
  return table;
}

void VirtualMemoryManager::initialize() {
  if (m_is_initialized)
    return;

  constexpr uintptr_t APIC_PHYS = 0xFEE00000;
  constexpr size_t APIC_SIZE = 0x4000;

  m_pml4 = alloc_table();
  if (!m_pml4) {
    kerror("VIRTUAL MEMORY", "Failed to allocate PML4");
    return;
  }

  map_ranges_iterative(PhysicalMemoryManager::the().m_memory_ranges.root());

  map_range(APIC_PHYS, APIC_PHYS, APIC_SIZE,
            PageFlags::Present | PageFlags::Writable |
                PageFlags::CacheDisabled);

  uintptr_t m_pml4_phys = reinterpret_cast<uintptr_t>(m_pml4);

  asm volatile("invlpg (%0)" : : "r"(m_pml4_phys) : "memory");

  m_is_initialized = true;
  klog("VIRTUAL MEMORY", "PML4 initialized at %p", m_pml4);
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys,
                                    uint64_t flags, size_t page_size) {
  uint64_t pml4_idx = (virt >> 39) & 0x1FF;
  uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
  uint64_t pd_idx = (virt >> 21) & 0x1FF;
  uint64_t pt_idx = (virt >> 12) & 0x1FF;

  uint64_t *pdpt;
  if (!(m_pml4[pml4_idx] & PageFlags::Present)) {
    pdpt = alloc_table();
    m_pml4[pml4_idx] = reinterpret_cast<uint64_t>(pdpt) | PageFlags::Present |
                       PageFlags::Writable;
  } else {
    pdpt = reinterpret_cast<uint64_t *>(m_pml4[pml4_idx] & ~0xFFF);
  }

  if (page_size == 0x40000000 && is_aligned(virt, 0x40000000) &&
      is_aligned(phys, 0x40000000)) {
    pdpt[pdpt_idx] = phys | flags | PageFlags::HugePage;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return;
  }

  uint64_t *pd;
  if (!(pdpt[pdpt_idx] & PageFlags::Present)) {
    pd = alloc_table();
    pdpt[pdpt_idx] = reinterpret_cast<uint64_t>(pd) | PageFlags::Present |
                     PageFlags::Writable;
  } else {
    pd = reinterpret_cast<uint64_t *>(pdpt[pdpt_idx] & ~0xFFF);
  }

  if (page_size == 0x200000 && is_aligned(virt, 0x200000) &&
      is_aligned(phys, 0x200000)) {
    pd[pd_idx] = phys | flags | PageFlags::HugePage;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return;
  }

  uint64_t *pt;
  if (!(pd[pd_idx] & PageFlags::Present)) {
    pt = alloc_table();
    pd[pd_idx] = reinterpret_cast<uint64_t>(pt) | PageFlags::Present |
                 PageFlags::Writable;
  } else {
    pt = reinterpret_cast<uint64_t *>(pd[pd_idx] & ~0xFFF);
  }

  if (!(pt[pt_idx] & PageFlags::Present)) {
    pt[pt_idx] = phys | flags;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
  }
}

void VirtualMemoryManager::map_range(uintptr_t virt_start, uintptr_t phys_start,
                                     size_t size, uint64_t flags) {
  size_t pages = (size + 0xFFF) / 0x1000;
  for (size_t i = 0; i < pages; ++i) {
    map_page(virt_start + i * 0x1000, phys_start + i * 0x1000, flags);
  }
}

uintptr_t VirtualMemoryManager::virt_to_phys(uintptr_t virt) {
  uint64_t pml4_idx = (virt >> 39) & 0x1FF;
  uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
  uint64_t pd_idx = (virt >> 21) & 0x1FF;
  uint64_t pt_idx = (virt >> 12) & 0x1FF;

  uint64_t *pdpt = reinterpret_cast<uint64_t *>(m_pml4[pml4_idx] & ~0xFFF);
  if (!pdpt)
    return 0;

  uint64_t *pd = reinterpret_cast<uint64_t *>(pdpt[pdpt_idx] & ~0xFFF);
  if (!pd)
    return 0;

  uint64_t *pt = reinterpret_cast<uint64_t *>(pd[pd_idx] & ~0xFFF);
  if (!pt)
    return 0;

  uint64_t entry = pt[pt_idx];
  if (!(entry & PageFlags::Present))
    return 0;

  return (entry & ~0xFFF) | (virt & 0xFFF);
}
