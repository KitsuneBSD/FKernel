#include <Kernel/Boot/boot_info.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/RegionSplitter/region_splitter.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>

VirtualMemoryManager::VirtualMemoryManager() : m_pml4(nullptr), m_pml4_phys(0) {
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Ctor (empty)");
  */
}

VirtualMemoryManager& VirtualMemoryManager::the() {
  static VirtualMemoryManager inst;
  return inst;
}

void VirtualMemoryManager::invlpg(uintptr_t addr) {
  invalid_tlb(addr);
}

void VirtualMemoryManager::flush_tlb() {
  write_on_cr3(reinterpret_cast<void*>(read_on_cr3()));
}

void VirtualMemoryManager::perform_initial_identity_mapping() {
  size_t pages = INITIAL_IDENTITY_MAPPING_SIZE / PAGE_SIZE;
  fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Identity mapping start: pages=%zu", pages);

  for (size_t i = 0; i < pages; i++) {
    uintptr_t phys = i * PAGE_SIZE;
    map_page(phys, phys, PageFlags::Present | PageFlags::Writable);
  }

  fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Identity mapping done");
}

void VirtualMemoryManager::initialize() {
  if (m_pml4) {
    fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Initialize skipped: already initialized");
    return;
  }

  // Aloca PML4 com uma página
  m_pml4_phys = PhysicalMemoryManager::the().alloc_page();
  if (m_pml4_phys == 0) {
    fk::algorithms::kfatal("VMM", "initialize: failed to allocate PML4 page");
    return;
  }
  m_kernel_pml4_phys = m_pml4_phys;

  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug(
      "VIRTUAL MEMORY MANAGER",
      "PML4 allocated: phys=%p",
      m_pml4_phys
  );
  */

  m_pml4 = reinterpret_cast<PageTable*>(m_pml4_phys);
  fk::memory::set(m_pml4, 0, PAGE_SIZE);

  perform_initial_identity_mapping();
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    uintptr_t start = fb.addr & ~0xFFFULL;
    uintptr_t end = (fb.addr + fb.pitch * fb.height + 0xFFF) & ~0xFFFULL;
    for (uintptr_t v = start; v < end; v += 0x1000) {
      map_page(v, v, PageFlags::Present | PageFlags::Writable);
    }
    fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Mapped framebuffer: %p - %p", (void*)start,
                         (void*)end);
  }

  write_on_cr3(static_cast<void*>(m_pml4));

  fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Initialize done: cr3=%p", m_pml4);
  m_is_initialized = true;
}

PageTable* VirtualMemoryManager::ensure_table(PageTable* parent, size_t index, PageFlags flags,
                                              bool& changed) {
  uint64_t user_bit = static_cast<uint64_t>(flags) & static_cast<uint64_t>(PageFlags::User);
  uint64_t write_bit = static_cast<uint64_t>(flags) & static_cast<uint64_t>(PageFlags::Writable);

  if (!(parent->entries[index] & static_cast<uint64_t>(PageFlags::Present))) {
    uintptr_t new_table = PhysicalMemoryManager::the().alloc_page();
    if (new_table == 0) {
      return nullptr;
    }
    fk::memory::set(reinterpret_cast<void*>(new_table), 0, PAGE_SIZE);
    parent->entries[index] =
        new_table | static_cast<uint64_t>(PageFlags::Present) | write_bit | user_bit;
    changed = true;
    return reinterpret_cast<PageTable*>(new_table);
  }

  uint64_t existing = parent->entries[index];

  // If we need the User bit but the existing intermediate entry is kernel-only,
  // copy the pointed-to table into a fresh page so we never modify shared kernel
  // page tables (PDPT_K / PD_K / their leaf PTs).
  if (user_bit && !(existing & static_cast<uint64_t>(PageFlags::User))) {
    uintptr_t old_addr = existing & 0x000FFFFFFFFFF000ULL;
    uintptr_t new_table = PhysicalMemoryManager::the().alloc_page();
    if (new_table == 0) return nullptr;
    fk::memory::copy(reinterpret_cast<void*>(new_table),
           reinterpret_cast<void*>(old_addr), PAGE_SIZE);
    parent->entries[index] = new_table | (existing & 0xFFFULL) | user_bit | write_bit;
    changed = true;
    return reinterpret_cast<PageTable*>(new_table);
  }

  uint64_t original = existing;
  parent->entries[index] |= (user_bit | write_bit);
  if (parent->entries[index] != original) {
    changed = true;
  }

  return reinterpret_cast<PageTable*>(parent->entries[index] & 0x000FFFFFFFFFF000ULL);
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags) {
  assert((virt % PAGE_SIZE) == 0);
  assert((phys % PAGE_SIZE) == 0);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  size_t pml4_idx = (virt >> 39) & 0x1FF;
  size_t pdpt_idx = (virt >> 30) & 0x1FF;
  size_t pd_idx = (virt >> 21) & 0x1FF;
  size_t pt_idx = (virt >> 12) & 0x1FF;

  bool changed_parents = false;

  PageTable* pdpt = ensure_table(m_pml4, pml4_idx, flags, changed_parents);
  if (!pdpt) {
    fk::algorithms::kwarn("VMM", "map_page: failed to ensure PDPT");
    return;
  }

  PageTable* pd = ensure_table(pdpt, pdpt_idx, flags, changed_parents);
  if (!pd) {
    fk::algorithms::kwarn("VMM", "map_page: failed to ensure PD");
    return;
  }

  PageTable* pt = ensure_table(pd, pd_idx, flags, changed_parents);
  if (!pt) {
    fk::algorithms::kwarn("VMM", "map_page: failed to ensure PT");
    return;
  }

  pt->entries[pt_idx] =
      phys | static_cast<uint64_t>(flags) | static_cast<uint64_t>(PageFlags::Present);

  if (changed_parents) {
    flush_tlb();
    return;
  }

  invlpg(virt);
}

void VirtualMemoryManager::unmap_page(uintptr_t virt) {
  fk::algorithms::kdebug("VMM", "unmap_page(%p)", (void*)virt);
  assert((virt % PAGE_SIZE) == 0);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  uint64_t* pte_ptr = get_pte(virt);
  if (pte_ptr && (*pte_ptr & static_cast<uint64_t>(PageFlags::Present))) {
    *pte_ptr = 0;
    invlpg(virt);
  }
}

void VirtualMemoryManager::protect_page(uintptr_t virt, PageFlags flags) {
  fk::algorithms::kdebug("VMM", "protect_page(%p, flags=0x%lx)", (void*)virt, (uint64_t)flags);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  uint64_t* pte = get_pte(virt, false);
  if (!pte || !(*pte & static_cast<uint64_t>(PageFlags::Present)))
    return;
  uintptr_t phys = *pte & 0x000FFFFFFFFFF000ULL;
  *pte = phys | static_cast<uint64_t>(flags);
  invlpg(virt);
}

uintptr_t VirtualMemoryManager::translate(uintptr_t virt) {
  assert((virt % PAGE_SIZE) == 0);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  size_t pml4_idx = (virt >> 39) & 0x1FF;
  size_t pdpt_idx = (virt >> 30) & 0x1FF;
  size_t pd_idx = (virt >> 21) & 0x1FF;
  size_t pt_idx = (virt >> 12) & 0x1FF;

  if (!(m_pml4->entries[pml4_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);

  if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);

  if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);

  if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  uintptr_t phys = (pt->entries[pt_idx] & 0x000FFFFFFFFFF000) + (virt & 0xFFF);
  return phys;
}

fk::core::Result<PageFlags, fk::core::Error> VirtualMemoryManager::get_page_flags(uintptr_t virt) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  size_t pml4_idx = (virt >> 39) & 0x1FF;
  size_t pdpt_idx = (virt >> 30) & 0x1FF;
  size_t pd_idx = (virt >> 21) & 0x1FF;
  size_t pt_idx = (virt >> 12) & 0x1FF;

  if (!(m_pml4->entries[pml4_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);
  if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);
  if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);
  if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;

  return static_cast<PageFlags>(pt->entries[pt_idx] & ~0x000FFFFFFFFFF000ULL);
}

uintptr_t clone_table_recursive(uintptr_t old_phys, int level, bool deep_copy) {
  uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
  if (!new_phys) return 0;

  PageTable* old_table = reinterpret_cast<PageTable*>(old_phys);
  PageTable* new_table = reinterpret_cast<PageTable*>(new_phys);
  fk::memory::set(new_table, 0, 0x1000);

  for (int i = 0; i < 512; ++i) {
    if (!(old_table->entries[i] & 1))
      continue; // Not present

    // Kernel-only mappings (no User bit) are shared by copying the entry
    if (!(old_table->entries[i] & 4)) {
      new_table->entries[i] = old_table->entries[i];
      continue;
    }

    // User mappings:
    if (level > 1) {
      uintptr_t old_sub = old_table->entries[i] & 0x000FFFFFFFFFF000;
      uintptr_t new_sub = clone_table_recursive(old_sub, level - 1, deep_copy);
      if (!new_sub) continue;
      new_table->entries[i] = new_sub | (old_table->entries[i] & 0xFFF);
    } else {
      // It's a PT, pointing to a page
      if (deep_copy) {
        uintptr_t old_page = old_table->entries[i] & 0x000FFFFFFFFFF000;
        uintptr_t new_page = PhysicalMemoryManager::the().alloc_page();
        if (!new_page) continue;
        fk::memory::copy(reinterpret_cast<void*>(new_page), reinterpret_cast<void*>(old_page), 0x1000);
        new_table->entries[i] = new_page | (old_table->entries[i] & 0xFFF);
      } else {
        new_table->entries[i] = 0;
      }
    }
  }
  return new_phys;
}

uintptr_t VirtualMemoryManager::create_address_space() {
  fk::algorithms::kdebug("VMM", "create_address_space()");
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  // Always clone from the kernel's root PML4, not from m_pml4_phys which tracks
  // the current task's (possibly user) address space and can become stale after frees.
  uintptr_t new_cr3 = clone_table_recursive(m_kernel_pml4_phys, 4, false);
  fk::algorithms::kdebug("VMM", "create_address_space() -> %p", (void*)new_cr3);
  return new_cr3;
}

uintptr_t VirtualMemoryManager::clone_address_space(uintptr_t source_cr3) {
  fk::algorithms::kdebug("VMM", "clone_address_space(%p)", (void*)source_cr3);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return clone_table_recursive(source_cr3, 4, true);
}

void VirtualMemoryManager::switch_address_space(uintptr_t cr3) {
  fk::algorithms::kdebug("VMM", "switch_address_space(%p)", (void*)cr3);
  if (cr3 == 0)
    return;
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pml4_phys = cr3;
  m_pml4 = reinterpret_cast<PageTable*>(cr3);
  write_on_cr3(reinterpret_cast<void*>(cr3));
}

void VirtualMemoryManager::free_address_space(uintptr_t cr3) {
  fk::algorithms::kdebug("VMM", "free_address_space(%p)", (void*)cr3);
  if (cr3 == 0 || cr3 == m_kernel_pml4_phys) return;

  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto* pml4 = reinterpret_cast<PageTable*>(cr3);

  // Walk only the user-space half of PML4 (entries 0-255 for 48-bit canonical)
  for (int pml4_i = 0; pml4_i < 256; ++pml4_i) {
    uint64_t pml4e = pml4->entries[pml4_i];
    if (!(pml4e & 1) || !(pml4e & 4)) continue;

    uintptr_t pdpt_phys = pml4e & 0x000FFFFFFFFFF000ULL;
    auto* pdpt = reinterpret_cast<PageTable*>(pdpt_phys);

    for (int pdpt_i = 0; pdpt_i < 512; ++pdpt_i) {
      uint64_t pdpte = pdpt->entries[pdpt_i];
      if (!(pdpte & 1) || !(pdpte & 4)) continue;

      uintptr_t pd_phys = pdpte & 0x000FFFFFFFFFF000ULL;
      auto* pd = reinterpret_cast<PageTable*>(pd_phys);

      for (int pd_i = 0; pd_i < 512; ++pd_i) {
        uint64_t pde = pd->entries[pd_i];
        if (!(pde & 1) || !(pde & 4)) continue;

        uintptr_t pt_phys = pde & 0x000FFFFFFFFFF000ULL;
        auto* pt = reinterpret_cast<PageTable*>(pt_phys);

        for (int pt_i = 0; pt_i < 512; ++pt_i) {
          uint64_t pte = pt->entries[pt_i];
          if (!(pte & 1) || !(pte & 4)) continue;
          PhysicalMemoryManager::the().free_page(pte & 0x000FFFFFFFFFF000ULL);
        }
        PhysicalMemoryManager::the().free_page(pt_phys);
      }
      PhysicalMemoryManager::the().free_page(pd_phys);
    }
    PhysicalMemoryManager::the().free_page(pdpt_phys);
  }
  PhysicalMemoryManager::the().free_page(cr3);
}

static PageTable* get_or_create_table(PageTable* parent, size_t index, bool create) {
  if (parent->entries[index] & static_cast<uint64_t>(PageFlags::Present))
    return reinterpret_cast<PageTable*>(parent->entries[index] & 0x000FFFFFFFFFF000);
  if (!create) return nullptr;
  uintptr_t new_table = PhysicalMemoryManager::the().alloc_page();
  if (!new_table) return nullptr;
  fk::memory::set(reinterpret_cast<void*>(new_table), 0, PAGE_SIZE);
  parent->entries[index] = new_table | static_cast<uint64_t>(PageFlags::Present)
                         | static_cast<uint64_t>(PageFlags::Writable)
                         | static_cast<uint64_t>(PageFlags::User);
  return reinterpret_cast<PageTable*>(new_table);
}

uint64_t* VirtualMemoryManager::get_pte(uintptr_t virt, bool create) {
  size_t pml4_idx = (virt >> 39) & 0x1FF;
  size_t pdpt_idx = (virt >> 30) & 0x1FF;
  size_t pd_idx = (virt >> 21) & 0x1FF;
  size_t pt_idx = (virt >> 12) & 0x1FF;

  PageTable* pdpt = get_or_create_table(m_pml4, pml4_idx, create);
  if (!pdpt) return nullptr;

  PageTable* pd = get_or_create_table(pdpt, pdpt_idx, create);
  if (!pd) return nullptr;

  PageTable* pt = get_or_create_table(pd, pd_idx, create);
  if (!pt) return nullptr;

  return &pt->entries[pt_idx];
}

static bool is_table_empty(PageTable* pt) {
  for (size_t i = 0; i < 512; ++i)
    if (pt->entries[i] != 0) return false;
  return true;
}

void VirtualMemoryManager::unmap_page_range(uintptr_t start, uintptr_t end) {
  for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
    uint64_t* pte_ptr = get_pte(addr);
    if (!pte_ptr) continue;
    if (!(*pte_ptr & static_cast<uint64_t>(PageFlags::Present))) continue;

    uint64_t frame = *pte_ptr & 0x000FFFFFFFFFF000;
    if (*pte_ptr & static_cast<uint64_t>(PageFlags::User))
      PhysicalMemoryManager::the().free_page(frame);

    *pte_ptr = 0;
    invlpg(addr);

    // Free empty intermediate tables
    size_t pml4_idx = (addr >> 39) & 0x1FF;
    size_t pdpt_idx = (addr >> 30) & 0x1FF;
    size_t pd_idx   = (addr >> 21) & 0x1FF;

    if (!(m_pml4->entries[pml4_idx] & 1)) continue;
    auto* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt->entries[pdpt_idx] & 1)) continue;
    auto* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pd->entries[pd_idx] & 1)) continue;
    auto* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000ULL);

    if (!is_table_empty(pt)) continue;
    PhysicalMemoryManager::the().free_page(reinterpret_cast<uintptr_t>(pt));
    pd->entries[pd_idx] = 0;

    if (!is_table_empty(pd)) continue;
    PhysicalMemoryManager::the().free_page(reinterpret_cast<uintptr_t>(pd));
    pdpt->entries[pdpt_idx] = 0;

    if (!is_table_empty(pdpt)) continue;
    PhysicalMemoryManager::the().free_page(reinterpret_cast<uintptr_t>(pdpt));
    m_pml4->entries[pml4_idx] = 0;
  }
}

fk::core::Result<int, fk::core::Error> VirtualMemoryManager::munmap(uintptr_t addr, size_t length) {
  if (addr % PAGE_SIZE != 0 || length == 0) {
    return fk::core::Error::InvalidParameter;
  }

  uintptr_t aligned_end = (addr + length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  fk::synchronization::ScopedLockIRQ lock(m_lock);

  // 1. Unmap from page tables
  unmap_page_range(addr, aligned_end);

  // 2. Update memory regions
  auto* current_task = SchedulerManager::the().current();
  if (!current_task)
    return fk::core::Error::PermissionDenied;

  auto& regions = current_task->resources.memory.regions.list;
  fkernel::RegionSplitter splitter(regions);
  splitter.split(addr, aligned_end);
  return 0;
}

void VirtualMemoryManager::map_range(uintptr_t start, uintptr_t size, PageFlags flags) {
  assert((start % PAGE_SIZE) == 0);
  assert((size % PAGE_SIZE) == 0);

  for (uintptr_t offset = 0; offset < size; offset += PAGE_SIZE) {
    // Here we assume identity mapping for simpler use cases or that
    // the caller wants to map virtual to physical identical addresses
    // This is commonly used for MMIO or kernel regions.
    map_page(start + offset, start + offset, flags);
  }
}
