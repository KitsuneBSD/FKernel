#include <Kernel/Boot/boot_info.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/RegionSplitter/region_splitter.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>

VirtualMemoryManager::VirtualMemoryManager() : m_pml4(nullptr), m_pml4_phys(0) {
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::klog("VIRT_MEM", "Ctor (empty)");
  */
}

VirtualMemoryManager& VirtualMemoryManager::the() {
  static VirtualMemoryManager inst;
  return inst;
}

void VirtualMemoryManager::invlpg(uintptr_t addr) {
  arch_invlpg(addr);
}

void VirtualMemoryManager::flush_tlb() {
  arch_write_cr3(reinterpret_cast<void*>(arch_read_cr3()));
}

void VirtualMemoryManager::perform_initial_identity_mapping() {
  size_t pages = INITIAL_IDENTITY_MAPPING_SIZE / PAGE_SIZE;
  fk::algorithms::klog("VIRT_MEM", "Identity mapping start: pages=%zu", pages);

  for (size_t i = 0; i < pages; i++) {
    uintptr_t phys = i * PAGE_SIZE;
    map_page(phys, phys, PageFlags::Present | PageFlags::Writable);
  }

  fk::algorithms::klog("VIRT_MEM", "Identity mapping done");
}

void VirtualMemoryManager::initialize() {
  if (m_pml4) {
    fk::algorithms::kwarn("VIRT_MEM", "Initialize skipped: already initialized");
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
      "VIRT_MEM",
      "PML4 allocated: phys=%p",
      m_pml4_phys
  );
  */

  m_pml4 = reinterpret_cast<PageTable*>(m_pml4_phys);
  fk::memory::set(m_pml4, 0, PAGE_SIZE);

  perform_initial_identity_mapping();
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    uintptr_t start = fb.addr & ~PAGE_FLAGS_MASK;
    uintptr_t end = (fb.addr + fb.pitch * fb.height + 0xFFF) & ~PAGE_FLAGS_MASK;
    for (uintptr_t v = start; v < end; v += PAGE_SIZE) {
      map_page(v, v, PageFlags::Present | PageFlags::Writable);
    }
    fk::algorithms::klog("VIRT_MEM", "Mapped framebuffer: %p - %p", (void*)start,
                         (void*)end);
  }

  arch_write_cr3(static_cast<void*>(m_pml4));

  fk::algorithms::klog("VIRT_MEM", "Initialize done: cr3=%p", m_pml4);
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
    uintptr_t old_addr = existing & PHYSICAL_ADDRESS_MASK;
    uintptr_t new_table = PhysicalMemoryManager::the().alloc_page();
    if (new_table == 0) return nullptr;
    fk::memory::copy(reinterpret_cast<void*>(new_table),
           reinterpret_cast<void*>(old_addr), PAGE_SIZE);
    parent->entries[index] = new_table | (existing & PAGE_FLAGS_MASK) | user_bit | write_bit;
    changed = true;
    return reinterpret_cast<PageTable*>(new_table);
  }

  uint64_t original = existing;
  parent->entries[index] |= (user_bit | write_bit);
  if (parent->entries[index] != original) {
    changed = true;
  }

  return reinterpret_cast<PageTable*>(parent->entries[index] & PHYSICAL_ADDRESS_MASK);
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags) {
  if ((virt % PAGE_SIZE) != 0 || (phys % PAGE_SIZE) != 0) {
    fk::algorithms::kwarn("VMM", "map_page: unaligned addr virt=%p phys=%p", (void*)virt, (void*)phys);
    return;
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  size_t pml4_idx = (virt >> PML4_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pdpt_idx = (virt >> PDPT_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pd_idx = (virt >> PD_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pt_idx = (virt >> PT_INDEX_SHIFT) & TABLE_INDEX_MASK;

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
  if ((virt % PAGE_SIZE) != 0) {
    fk::algorithms::kwarn("VMM", "unmap_page: unaligned addr %p", (void*)virt);
    return;
  }
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
  bool changed = false;
  PageTable* pt = ensure_table(
      ensure_table(
          ensure_table(m_pml4, (virt >> PML4_INDEX_SHIFT) & TABLE_INDEX_MASK, flags, changed),
          (virt >> PDPT_INDEX_SHIFT) & TABLE_INDEX_MASK, flags, changed),
      (virt >> PD_INDEX_SHIFT) & TABLE_INDEX_MASK, flags, changed);
  if (!pt) return;
  size_t pt_idx = (virt >> PT_INDEX_SHIFT) & TABLE_INDEX_MASK;
  if (!(pt->entries[pt_idx] & static_cast<uint64_t>(PageFlags::Present))) return;
  uintptr_t phys = pt->entries[pt_idx] & PHYSICAL_ADDRESS_MASK;
  pt->entries[pt_idx] = phys | static_cast<uint64_t>(flags) | static_cast<uint64_t>(PageFlags::Present);
  if (changed) { flush_tlb(); return; }
  invlpg(virt);
}

uintptr_t VirtualMemoryManager::translate(uintptr_t virt) {
  if ((virt % PAGE_SIZE) != 0) {
    fk::algorithms::kwarn("VMM", "translate: unaligned addr %p", (void*)virt);
    return 0;
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  size_t pml4_idx = (virt >> PML4_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pdpt_idx = (virt >> PDPT_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pd_idx = (virt >> PD_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pt_idx = (virt >> PT_INDEX_SHIFT) & TABLE_INDEX_MASK;

  if (!(m_pml4->entries[pml4_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & PHYSICAL_ADDRESS_MASK);

  if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & PHYSICAL_ADDRESS_MASK);

  if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & PHYSICAL_ADDRESS_MASK);

  if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present)) {
    return 0;
  }

  uintptr_t phys = (pt->entries[pt_idx] & PHYSICAL_ADDRESS_MASK) + (virt & 0xFFF);
  return phys;
}

fk::core::Result<PageFlags, fk::core::Error> VirtualMemoryManager::get_page_flags(uintptr_t virt) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  size_t pml4_idx = (virt >> PML4_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pdpt_idx = (virt >> PDPT_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pd_idx = (virt >> PD_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pt_idx = (virt >> PT_INDEX_SHIFT) & TABLE_INDEX_MASK;

  if (!(m_pml4->entries[pml4_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & PHYSICAL_ADDRESS_MASK);
  if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & PHYSICAL_ADDRESS_MASK);
  if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;
  PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & PHYSICAL_ADDRESS_MASK);
  if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present))
    return fk::core::Error::NotFound;

  return static_cast<PageFlags>(pt->entries[pt_idx] & ~PHYSICAL_ADDRESS_MASK);
}

uintptr_t clone_table_recursive(uintptr_t old_phys, int level, bool deep_copy) {
  uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
  if (!new_phys) return 0;

  PageTable* old_table = reinterpret_cast<PageTable*>(old_phys);
  PageTable* new_table = reinterpret_cast<PageTable*>(new_phys);
  fk::memory::set(new_table, 0, PAGE_SIZE);

  for (size_t i = 0; i < PT_ENTRIES; ++i) {
    if (!(old_table->entries[i] & 1))
      continue; // Not present

    // Kernel-only mappings (no User bit) are shared by copying the entry
    if (!(old_table->entries[i] & 4)) {
      new_table->entries[i] = old_table->entries[i];
      continue;
    }

    // User mappings:
    if (level > 1) {
      uintptr_t old_sub = old_table->entries[i] & PHYSICAL_ADDRESS_MASK;
      uintptr_t new_sub = clone_table_recursive(old_sub, level - 1, deep_copy);
      if (!new_sub) continue;
      new_table->entries[i] = new_sub | (old_table->entries[i] & 0xFFF);
    } else {
      // It's a PT, pointing to a page
      if (deep_copy) {
        uintptr_t old_page = old_table->entries[i] & PHYSICAL_ADDRESS_MASK;
        uint64_t flags = old_table->entries[i] & 0xFFF;
        // CoW: share the physical frame, clear Writable in both parent and child
        if (flags & static_cast<uint64_t>(PageFlags::Writable)) {
          flags &= ~static_cast<uint64_t>(PageFlags::Writable);
          old_table->entries[i] = old_page | flags;
        }
        new_table->entries[i] = old_page | flags;
        PhysicalMemoryManager::the().increment_refcount(old_page);
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
  return clone_table_recursive(m_kernel_pml4_phys, PAGE_TABLE_LEVELS, false);
}

uintptr_t VirtualMemoryManager::clone_address_space(uintptr_t source_cr3) {
  fk::algorithms::kdebug("VMM", "clone_address_space(%p)", (void*)source_cr3);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return clone_table_recursive(source_cr3, PAGE_TABLE_LEVELS, true);
}

void VirtualMemoryManager::switch_address_space(uintptr_t cr3) {
  fk::algorithms::kdebug("VMM", "switch_address_space(%p)", (void*)cr3);
  if (cr3 == 0)
    return;
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_pml4_phys = cr3;
  m_pml4 = reinterpret_cast<PageTable*>(cr3);
  arch_write_cr3(reinterpret_cast<void*>(cr3));
}

void VirtualMemoryManager::free_address_space(uintptr_t cr3) {
  fk::algorithms::kdebug("VMM", "free_address_space(%p)", (void*)cr3);
  if (cr3 == 0 || cr3 == m_kernel_pml4_phys) return;

  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto* pml4 = reinterpret_cast<PageTable*>(cr3);

  // Walk only the user-space half of PML4 (entries 0-255 for 48-bit canonical)
  for (size_t pml4_i = 0; pml4_i < PT_ENTRIES / 2; ++pml4_i) {
    uint64_t pml4e = pml4->entries[pml4_i];
    if (!(pml4e & 1) || !(pml4e & 4)) continue;

    uintptr_t pdpt_phys = pml4e & PHYSICAL_ADDRESS_MASK;
    auto* pdpt = reinterpret_cast<PageTable*>(pdpt_phys);

    for (int pdpt_i = 0; pdpt_i < 512; ++pdpt_i) {
      uint64_t pdpte = pdpt->entries[pdpt_i];
      if (!(pdpte & 1) || !(pdpte & 4)) continue;

      uintptr_t pd_phys = pdpte & PHYSICAL_ADDRESS_MASK;
      auto* pd = reinterpret_cast<PageTable*>(pd_phys);

      for (int pd_i = 0; pd_i < 512; ++pd_i) {
        uint64_t pde = pd->entries[pd_i];
        if (!(pde & 1) || !(pde & 4)) continue;

        uintptr_t pt_phys = pde & PHYSICAL_ADDRESS_MASK;
        auto* pt = reinterpret_cast<PageTable*>(pt_phys);

        for (int pt_i = 0; pt_i < 512; ++pt_i) {
          uint64_t pte = pt->entries[pt_i];
          if (!(pte & 1) || !(pte & 4)) continue;
          PhysicalMemoryManager::the().free_page(pte & PHYSICAL_ADDRESS_MASK);
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
    return reinterpret_cast<PageTable*>(parent->entries[index] & PHYSICAL_ADDRESS_MASK);
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
  size_t pml4_idx = (virt >> PML4_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pdpt_idx = (virt >> PDPT_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pd_idx = (virt >> PD_INDEX_SHIFT) & TABLE_INDEX_MASK;
  size_t pt_idx = (virt >> PT_INDEX_SHIFT) & TABLE_INDEX_MASK;

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

    uint64_t frame = *pte_ptr & PHYSICAL_ADDRESS_MASK;
    if (*pte_ptr & static_cast<uint64_t>(PageFlags::User))
      PhysicalMemoryManager::the().free_page(frame);

    *pte_ptr = 0;
    invlpg(addr);

    // Free empty intermediate tables
    size_t pml4_idx = (addr >> 39) & 0x1FF;
    size_t pdpt_idx = (addr >> 30) & 0x1FF;
    size_t pd_idx   = (addr >> 21) & 0x1FF;

    if (!(m_pml4->entries[pml4_idx] & 1)) continue;
    auto* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & PHYSICAL_ADDRESS_MASK);

    if (!(pdpt->entries[pdpt_idx] & 1)) continue;
    auto* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & PHYSICAL_ADDRESS_MASK);

    if (!(pd->entries[pd_idx] & 1)) continue;
    auto* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & PHYSICAL_ADDRESS_MASK);

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
  if ((start % PAGE_SIZE) != 0 || (size % PAGE_SIZE) != 0) {
    fk::algorithms::kwarn("VMM", "map_range: unaligned start=%p size=%zu", (void*)start, size);
    return;
  }

  for (uintptr_t offset = 0; offset < size; offset += PAGE_SIZE) {
    // Here we assume identity mapping for simpler use cases or that
    // the caller wants to map virtual to physical identical addresses
    // This is commonly used for MMIO or kernel regions.
    map_page(start + offset, start + offset, flags);
  }
}

void VirtualMemoryManager::extend_direct_map() {
  fk::algorithms::klog("VMM", "Extending direct map at %p", (void*)KERNEL_VIRT_BASE);

  size_t total_memory = PhysicalMemoryManager::the().total_memory();
  size_t aligned_total = (total_memory + PAGE_SIZE_2M - 1) & ~(PAGE_SIZE_2M - 1);

  fk::algorithms::klog("VMM", "Direct map: %zu MB physical memory", total_memory / (1024 * 1024));

  fk::synchronization::ScopedLockIRQ lock(m_lock);

  size_t pml4_idx = (KERNEL_VIRT_BASE >> 39) & 0x1FF;
  uint64_t pml4e = m_pml4->entries[pml4_idx];

  PageTable* pdpt;
  if (!(pml4e & static_cast<uint64_t>(PageFlags::Present))) {
    uintptr_t pdpt_phys = PhysicalMemoryManager::the().alloc_page();
    if (!pdpt_phys) {
      fk::algorithms::kerror("VMM", "extend_direct_map: failed to allocate PDPT");
      return;
    }
    fk::memory::set(reinterpret_cast<void*>(pdpt_phys), 0, PAGE_SIZE);
    m_pml4->entries[pml4_idx] = pdpt_phys | static_cast<uint64_t>(PageFlags::Present)
                                                  | static_cast<uint64_t>(PageFlags::Writable);
    pdpt = reinterpret_cast<PageTable*>(pdpt_phys);
  } else {
    pdpt = reinterpret_cast<PageTable*>(pml4e & PHYSICAL_ADDRESS_MASK);
  }

  for (uintptr_t offset = 0; offset < aligned_total; offset += PAGE_SIZE_2M) {
    size_t pdpt_idx = (offset >> 30) & 0x1FF;
    size_t pd_idx = (offset >> 21) & 0x1FF;

    uint64_t pdpte = pdpt->entries[pdpt_idx];

    PageTable* pd;
    if (!(pdpte & static_cast<uint64_t>(PageFlags::Present))) {
      uintptr_t pd_phys = PhysicalMemoryManager::the().alloc_page();
      if (!pd_phys) {
        fk::algorithms::kerror("VMM", "extend_direct_map: out of memory at offset %p", (void*)offset);
        return;
      }
      fk::memory::set(reinterpret_cast<void*>(pd_phys), 0, PAGE_SIZE);
      pdpt->entries[pdpt_idx] = pd_phys | static_cast<uint64_t>(PageFlags::Present)
                                         | static_cast<uint64_t>(PageFlags::Writable);
      pd = reinterpret_cast<PageTable*>(pd_phys);
    } else {
      pd = reinterpret_cast<PageTable*>(pdpte & PHYSICAL_ADDRESS_MASK);
    }

    pd->entries[pd_idx] = offset | static_cast<uint64_t>(PageFlags::Present)
                                 | static_cast<uint64_t>(PageFlags::Writable)
                                 | static_cast<uint64_t>(PageFlags::HugePage);
  }

  flush_tlb();

  fk::algorithms::klog("VMM", "Direct map extended: %zu MB", aligned_total / (1024 * 1024));
}

uintptr_t VirtualMemoryManager::create_kpti_user_pml4(uintptr_t kernel_pml4_phys) {
  // Allocate a fresh PML4 page for the KPTI user shadow.
  uintptr_t shadow_phys = PhysicalMemoryManager::the().alloc_page();
  if (!shadow_phys) return 0;
  auto* shadow = reinterpret_cast<PageTable*>(shadow_phys);
  fk::memory::set(shadow, 0, PAGE_SIZE);

  const auto* kpml4 = reinterpret_cast<const PageTable*>(kernel_pml4_phys);

  // Copy all PML4 entries.
  // Strategy: include user-accessible entries (User flag set) and the kernel's
  // global identity mapping entries (no User flag, but needed for the syscall
  // trampoline to execute in ring 0 after SYSCALL, before the CR3 swap completes).
  // Specifically: entries without User bit ARE included so ring-0 can access
  // kernel text/GS data immediately after SYSCALL. Per-process kernel stacks
  // and heap pages are mixed into the same identity range; full isolation
  // requires removing those specific PTEs, which is a future refinement.
  for (size_t i = 0; i < PT_ENTRIES / 2; ++i) {
    uint64_t entry = kpml4->entries[i];
    if (entry & static_cast<uint64_t>(PageFlags::Present))
      shadow->entries[i] = entry;
  }

  return shadow_phys;
}

void VirtualMemoryManager::free_kpti_user_pml4(uintptr_t user_pml4_phys) {
  // Only free the PML4 page itself; the PDPT/PD/PT pages are shared with
  // the kernel PML4 and must NOT be freed here.
  if (user_pml4_phys) PhysicalMemoryManager::the().free_page(user_pml4_phys);
}
