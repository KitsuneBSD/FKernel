#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Hardware/Acpi/topology_manager.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_zone.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/memory.h>

extern "C" uint8_t __kernel_start[];
extern "C" uint8_t __kernel_end[];
extern "C" uint8_t __heap_start[];
extern "C" uint8_t __heap_end[];
extern "C" uint64_t __pmm_bitmap_start[];
extern "C" uint64_t __pmm_bitmap_end[];

PhysicalZone* PhysicalMemoryManager::create_zone(uintptr_t base, size_t length, ZoneType type,
                                                 uint64_t* bitmap_storage, size_t bitmap_bits) {
  assert(m_zone_count < MAX_PHYSICAL_ZONES);
  assert((base % FRAME_SIZE) == 0);
  assert((length % FRAME_SIZE) == 0);
  assert(bitmap_storage != nullptr);
  assert(bitmap_bits > 0);

  PhysicalZone& pz = m_zones[m_zone_count];

  new (&pz.bitmap) fk::containers::Bitmap<uint64_t>(bitmap_storage, bitmap_bits);

  pz.zone.populate_zone(base, length, type);
  pz.buddy.add_range(base, length);

  // Set proximity domain using TopologyManager
  pz.proximity_domain = fkernel::acpi::TopologyManager::the().get_node_for_paddr(base);

  m_zone_count++;
  return &pz;
}

void PhysicalMemoryManager::process_range(uintptr_t base, uintptr_t end, uint64_t*& bitmap_cursor,
                                          size_t bitmap_words_remaining) {
  while (base < end) {
    ZoneType type = classify_zone(base);
    uintptr_t limit = zone_limit(type);

    uintptr_t zone_end = end < limit ? end : limit;
    size_t length = zone_end - base;

    if (length == 0)
      break;

    size_t frames = length / FRAME_SIZE;
    size_t bitmap_bits = frames;
    size_t bitmap_words = (bitmap_bits + 63) / 64;

    assert(bitmap_words <= bitmap_words_remaining);

    create_zone(base, length, type, bitmap_cursor, bitmap_bits);

    bitmap_cursor += bitmap_words;
    bitmap_words_remaining -= bitmap_words;

    base = zone_end;
  }
}

void PhysicalMemoryManager::reserve_range(uintptr_t base, size_t length) {
  if (length == 0)
    return;

  uintptr_t start = fk::utilities::align_down(base, FRAME_SIZE);
  uintptr_t end = fk::utilities::align_up(base + length, FRAME_SIZE);

  for (uintptr_t addr = start; addr < end; addr += FRAME_SIZE) {
    PhysicalZone* pz = find_zone_for_paddr(addr);
    if (pz) {
      size_t frame = (addr - pz->zone.base()) / FRAME_SIZE;
      if (!pz->bitmap.get(frame)) {
        pz->bitmap.set(frame, true);
        m_free_memory -= FRAME_SIZE;
      }
    }
  }
}

void PhysicalMemoryManager::initialize() {
  assert(!m_is_initialized);
  assert(boot::BootInfo::the().is_initialized() &&
         "BootInfo must be initialized before PhysicalMemoryManager!");

  fk::algorithms::klog("PHYS_MEM", "Initializing Physical Memory Manager");

  // Initialize TopologyManager to discover NUMA layout
  fkernel::acpi::TopologyManager::the().initialize();


  uint64_t* bitmap_cursor = __pmm_bitmap_start;
  size_t bitmap_words_total = (__pmm_bitmap_end - __pmm_bitmap_start);
  size_t bitmap_words_remaining = bitmap_words_total;

  auto* memory_map = boot::BootInfo::the().get_memory_map_iterator();
  assert(memory_map && "Memory map iterator is null!");

  memory_map->reset();
  while (memory_map->has_next()) {
    auto entry = memory_map->next();

    if (!entry.is_available)
      continue;

    uintptr_t base = fk::utilities::align_up(entry.base_addr, FRAME_SIZE);
    uintptr_t end = fk::utilities::align_down(entry.base_addr + entry.length, FRAME_SIZE);

    if (end <= base)
      continue;

    size_t usable = end - base;
    m_total_memory += usable;
    m_free_memory += usable;

    process_range(base, end, bitmap_cursor, bitmap_words_remaining);
  }

  // Reserve Kernel range
  reserve_range(reinterpret_cast<uintptr_t>(__kernel_start),
                reinterpret_cast<uintptr_t>(__kernel_end) -
                    reinterpret_cast<uintptr_t>(__kernel_start));

  // Reserve Heap range
  reserve_range(reinterpret_cast<uintptr_t>(__heap_start),
                reinterpret_cast<uintptr_t>(__heap_end) -
                    reinterpret_cast<uintptr_t>(__heap_start));

  // Reserve PMM Bitmap range
  reserve_range(reinterpret_cast<uintptr_t>(__pmm_bitmap_start),
                reinterpret_cast<uintptr_t>(__pmm_bitmap_end) -
                    reinterpret_cast<uintptr_t>(__pmm_bitmap_start));

  // Reserve AP trampoline page (0x8000-0x9000)
  reserve_range(0x8000, 0x1000);

  // Reserve Multiboot data
  void* mb_ptr = boot::BootInfo::the().get_raw_multiboot_ptr();
  if (mb_ptr) {
    uint32_t size = *reinterpret_cast<uint32_t*>(mb_ptr);
    reserve_range(reinterpret_cast<uintptr_t>(mb_ptr), size);
  }

  // Reserve Modules
  auto& modules = boot::BootInfo::the().get_modules();
  for (const auto& mod : modules) {
    reserve_range(mod.start, mod.end - mod.start);
  }

  m_is_initialized = true;

  fk::algorithms::klog("PHYS_MEM",
                       "Initialized: zones=%lu total=%lu MB, free=%lu MB", m_zone_count,
                       m_total_memory / (1024 * 1024), m_free_memory / (1024 * 1024));
}

void PhysicalMemoryManager::reconcile_buddies() {
  fk::algorithms::klog("PHYS_MEM", "Reconciling buddy allocators with bitmap");

  for (size_t i = 0; i < m_zone_count; ++i) {
    m_zones[i].buddy.initialize_from_bitmap(m_zones[i].bitmap, m_zones[i].zone.base());
  }

  fk::algorithms::klog("PHYS_MEM", "Buddy reconciliation complete");

  // Allocate CoW refcount arrays for each zone (direct map now available)
  for (size_t i = 0; i < m_zone_count; ++i) {
    auto& pz = m_zones[i];
    size_t frames = pz.zone.length() / FRAME_SIZE;
    size_t bytes = frames * sizeof(uint16_t);
    size_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    uintptr_t arr_phys = 0;

    if (pages == 1) {
      arr_phys = alloc_page(ZoneType::NORMAL, pz.proximity_domain);
    } else {
      size_t order = size_to_order(bytes);
      arr_phys = alloc_contiguous(order, ZoneType::NORMAL, pz.proximity_domain);
    }

    if (!arr_phys) {
      fk::algorithms::kwarn("PHYS_MEM",
                            "Failed to allocate CoW refcount array for zone %zu (frames=%zu)",
                            i, frames);
      continue;
    }

    auto* refs = reinterpret_cast<uint16_t*>(arr_phys + KERNEL_VIRT_BASE);
    fk::memory::set(refs, 0, bytes);
    pz.cow_refcounts = refs;
    pz.cow_frame_count = frames;
  }
}

PhysicalZone* PhysicalMemoryManager::find_zone_for_paddr(uintptr_t phys) {
  for (size_t i = 0; i < m_zone_count; ++i) {
    auto& z = m_zones[i].zone;
    if (phys >= z.base() && phys < z.base() + z.length()) {
      return &m_zones[i];
    }
  }

  fk::algorithms::kwarn("PHYS_MEM", "No zone found for phys=%p", phys);

  return nullptr;
}

PhysicalZone* PhysicalMemoryManager::select_zone(ZoneType preferred, uint32_t preferred_node) {
  // 1. Try to find preferred zone type in preferred node
  for (size_t i = 0; i < m_zone_count; ++i) {
    if (m_zones[i].zone.type() == preferred && m_zones[i].proximity_domain == preferred_node) {
      return &m_zones[i];
    }
  }

  // 2. Try to find any zone in preferred node
  for (size_t i = 0; i < m_zone_count; ++i) {
    if (m_zones[i].proximity_domain == preferred_node) {
      return &m_zones[i];
    }
  }

  // 3. Fallback: Try to find preferred zone type in ANY node
  for (size_t i = 0; i < m_zone_count; ++i) {
    if (m_zones[i].zone.type() == preferred) {
      return &m_zones[i];
    }
  }

  // 4. Ultimate Fallback: Any available NORMAL zone
  for (size_t i = 0; i < m_zone_count; ++i) {
    if (m_zones[i].zone.type() == ZoneType::NORMAL) {
      return &m_zones[i];
    }
  }

  if (m_zone_count > 0) {
    return &m_zones[0];
  }

  fk::algorithms::kwarn("PHYS_MEM", "No zones available");
  return nullptr;
}

uintptr_t PhysicalMemoryManager::alloc_page_internal(ZoneType preferred,
                                                     uint32_t preferred_node) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = select_zone(preferred, preferred_node);
  if (!pz) {
    fk::algorithms::kwarn("PHYS_MEM", "Alloc_page: No zone available");
    return 0;
  }

  ssize_t frame = pz->bitmap.alloc();
  if (frame >= 0) {
    uintptr_t phys = pz->zone.base() + (frame * FRAME_SIZE);
    pz->buddy.invalidate_page(phys);
    m_free_memory -= FRAME_SIZE;
    if (pz->cow_refcounts) {
      pz->cow_refcounts[frame] = 1;
    }
    return phys;
  }

  fk::algorithms::kwarn("PHYS_MEM", "Alloc_page: No free pages in zone type %d",
                        (int)pz->zone.type());
  return 0;
}

uintptr_t PhysicalMemoryManager::alloc_page(ZoneType preferred, uint32_t preferred_node) {
  if (!m_is_initialized) {
    fk::algorithms::kwarn("PHYS_MEM", "Alloc_page called before init");
    return 0;
  }
  return alloc_page_internal(preferred, preferred_node);
}

void PhysicalMemoryManager::free_page(uintptr_t phys) {
  if (!m_is_initialized) {
    fk::algorithms::kwarn("PHYS_MEM", "Free_page called before init");
    return;
  }
  if ((phys % FRAME_SIZE) != 0) {
    fk::algorithms::kwarn("PHYS_MEM", "free_page: unaligned phys=%p", (void*)phys);
    return;
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz) {
    fk::algorithms::kerror("PHYS_MEM", "free_page: no zone found for address %p",
                           phys);
    return;
  }

  size_t frame = (phys - pz->zone.base()) / FRAME_SIZE;

  if (pz->cow_refcounts && pz->cow_refcounts[frame] > 0) {
    pz->cow_refcounts[frame]--;
    if (pz->cow_refcounts[frame] > 0) {
      return;
    }
  }

  pz->bitmap.clear(frame);
  pz->buddy.free(reinterpret_cast<void*>(phys), MIN_ORDER);
  m_free_memory += FRAME_SIZE;
}

uintptr_t PhysicalMemoryManager::alloc_contiguous_internal(size_t order, ZoneType preferred,
                                                          uint32_t preferred_node) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = select_zone(preferred, preferred_node);
  if (!pz) {
    fk::algorithms::kwarn("PHYS_MEM", "alloc_contiguous: No zone available");
    return 0;
  }

  void* block = pz->buddy.alloc(order);
  if (!block) {
    fk::algorithms::kwarn("PHYS_MEM",
                          "alloc_contiguous: Buddy allocation failed for order "
                          "%lu in zone type %d",
                          order, (int)pz->zone.type());
    return 0;
  }

  uintptr_t phys = reinterpret_cast<uintptr_t>(block);
  size_t effective_order = order < MIN_ORDER ? MIN_ORDER : order;
  size_t page_count = order_to_size(effective_order) / FRAME_SIZE;
  uintptr_t zone_base = pz->zone.base();
  for (size_t i = 0; i < page_count; i++) {
    size_t frame = (phys - zone_base + i * FRAME_SIZE) / FRAME_SIZE;
    pz->bitmap.set(frame, true);
  }

  if (pz->cow_refcounts) {
    size_t first_frame = (phys - zone_base) / FRAME_SIZE;
    for (size_t i = 0; i < page_count; i++) {
      pz->cow_refcounts[first_frame + i] = 1;
    }
  }

  m_free_memory -= (FRAME_SIZE << order);
  return phys;
}

uintptr_t PhysicalMemoryManager::alloc_contiguous(size_t order, ZoneType preferred,
                                                  uint32_t preferred_node) {
  if (!m_is_initialized) {
    fk::algorithms::kwarn("PHYS_MEM", "alloc_contiguous called before init");
    return 0;
  }
  return alloc_contiguous_internal(order, preferred, preferred_node);
}

void PhysicalMemoryManager::free_contiguous(uintptr_t phys, size_t order) {
  if (!m_is_initialized) {
    fk::algorithms::kwarn("PHYS_MEM", "free_contiguous called before init");
    return;
  }
  if ((phys % FRAME_SIZE) != 0) {
    fk::algorithms::kwarn("PHYS_MEM", "free_contiguous: unaligned phys=%p", (void*)phys);
    return;
  }
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz) {
    fk::algorithms::kerror("PHYS_MEM",
                           "free_contiguous: no zone found for address %p", phys);
    return;
  }

  size_t effective_order = order < MIN_ORDER ? MIN_ORDER : order;
  size_t page_count = order_to_size(effective_order) / FRAME_SIZE;
  uintptr_t zone_base = pz->zone.base();
  for (size_t i = 0; i < page_count; i++) {
    size_t frame = (phys - zone_base + i * FRAME_SIZE) / FRAME_SIZE;
    if (pz->cow_refcounts && pz->cow_refcounts[frame] > 0) {
      if (--pz->cow_refcounts[frame] > 0) continue;
    }
    pz->bitmap.clear(frame);
  }

  pz->buddy.free(reinterpret_cast<void*>(phys), order);
  m_free_memory += (FRAME_SIZE << order);
}

void PhysicalMemoryManager::increment_refcount(uintptr_t phys) {
  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz || !pz->cow_refcounts) return;
  size_t frame = (phys - pz->zone.base()) / FRAME_SIZE;
  if (frame >= pz->cow_frame_count) return;
  pz->cow_refcounts[frame]++;
}

uint16_t PhysicalMemoryManager::decrement_refcount(uintptr_t phys) {
  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz || !pz->cow_refcounts) return 0;
  size_t frame = (phys - pz->zone.base()) / FRAME_SIZE;
  if (frame >= pz->cow_frame_count) return 0;
  if (pz->cow_refcounts[frame] > 0)
    pz->cow_refcounts[frame]--;
  return pz->cow_refcounts[frame];
}

uint16_t PhysicalMemoryManager::get_refcount(uintptr_t phys) const {
  for (size_t i = 0; i < m_zone_count; ++i) {
    auto& z = m_zones[i].zone;
    if (phys >= z.base() && phys < z.base() + z.length()) {
      if (!m_zones[i].cow_refcounts) return 1;
      size_t frame = (phys - z.base()) / FRAME_SIZE;
      if (frame >= m_zones[i].cow_frame_count) return 1;
      uint16_t rc = m_zones[i].cow_refcounts[frame];
      if (rc == 0) return 1;
      return rc;
    }
  }
  return 1;
}