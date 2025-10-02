#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

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

  m_pml4 = alloc_table();
  if (!m_pml4) {
    kerror("VIRTUAL MEMORY", "Failed to allocate PML4");
    return;
  }

  for (auto *node = PhysicalMemoryManager::the().m_memory_ranges.root(); node;
       /* iter */) {
    auto &range = node->value();
    // NOTE: Identity map of reserved value
    if (range.m_type != MemoryType::Usable) {
      map_range(range.m_start, range.m_start, range.m_end - range.m_start,
                PageFlags::Present | PageFlags::Writable);
    }
    node = node->right();
  }

  uintptr_t m_pml4_phys = reinterpret_cast<uintptr_t>(m_pml4);

  asm volatile("invlpg (%0)" : : "r"(m_pml4_phys) : "memory");

  m_is_initialized = true;
  klog("VIRTUAL MEMORY", "PML4 initialized at %p", m_pml4);
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys,
                                    uint64_t flags) {
  // kprintf("Map [ %p -> %p ]\n", phys, virt);
  uint64_t pml4_idx = (virt >> 39) & 0x1FF;
  uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
  uint64_t pd_idx = (virt >> 21) & 0x1FF;
  uint64_t pt_idx = (virt >> 12) & 0x1FF;

  uint64_t *pdpt = reinterpret_cast<uint64_t *>(m_pml4[pml4_idx] & ~0xFFF);
  if (!pdpt) {
    pdpt = alloc_table();
    m_pml4[pml4_idx] = reinterpret_cast<uint64_t>(pdpt) | PageFlags::Present |
                       PageFlags::Writable;
  }

  uint64_t *pd = reinterpret_cast<uint64_t *>(pdpt[pdpt_idx] & ~0xFFF);
  if (!pd) {
    pd = alloc_table();
    pdpt[pdpt_idx] = reinterpret_cast<uint64_t>(pd) | PageFlags::Present |
                     PageFlags::Writable;
  }

  uint64_t *pt = reinterpret_cast<uint64_t *>(pd[pd_idx] & ~0xFFF);
  if (!pt) {
    pt = alloc_table();
    pd[pd_idx] = reinterpret_cast<uint64_t>(pt) | PageFlags::Present |
                 PageFlags::Writable;
  }

  pt[pt_idx] = phys | flags;

  asm volatile("invlpg (%0)" : : "r"(m_pml4) : "memory");
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
