#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Memory/VirtualMemory/VirtualMemoryManager.h>
#include <Kernel/Memory/PhysicalMemory/PhysicalMemoryManager.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

extern "C" void write_on_cr3(void *pml4);

void MemoryManager::map_page(uintptr_t virt, uintptr_t phys, uint64_t flags,
                             [[maybe_unused]] uint64_t page_size) {

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

  fk::algorithms::kdebug("MEMORY MANAGER",
                         "Mapped page V:0x%lx -> P:0x%lx (flags 0x%lx)", virt,
                         phys, flags);
}

void MemoryManager::map_range(uintptr_t virt_start, uintptr_t phys_start,
                              size_t size, uint64_t flags) {
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    map_page(virt_start + offset, phys_start + offset, flags, PAGE_SIZE);
  }
}

uint64_t *MemoryManager::alloc_table() {
  void *page =
      reinterpret_cast<void *>(PhysicalMemoryManager::the().alloc_page());
  if (!page) {
    fk::algorithms::kwarn("MEMORY MANAGER",
                          "Failed to allocate page for table.");
    return nullptr;
  }

  uint64_t *table = reinterpret_cast<uint64_t *>(page);
  memset(table, 0, PAGE_SIZE);
  return table;
}

void MemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (m_is_initialized) {
    fk::algorithms::kwarn("MEMORY MANAGER", "Already initialized.");
    return;
  }

  PhysicalMemoryManager::the().initialize(mmap);

  fk::algorithms::klog("MEMORY MANAGER", "Memory Manager initialized");
  m_is_initialized = true;
}
