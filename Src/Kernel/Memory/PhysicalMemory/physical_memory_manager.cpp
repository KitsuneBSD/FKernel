#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Hardware/Acpi/topology_manager.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_zone.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/new.h>

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

  fk::algorithms::klog("PHYSICAL MEMORY MANAGER", "Initializing Physical Memory Manager");

  // Initialize TopologyManager to discover NUMA layout
  fkernel::acpi::TopologyManager::the().initialize();

  extern uint8_t __kernel_start[];
  extern uint8_t __kernel_end[];
  extern uint8_t __heap_start[];
  extern uint8_t __heap_end[];
  extern uint64_t __pmm_bitmap_start[];
  extern uint64_t __pmm_bitmap_end[];

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

  fk::algorithms::klog("PHYSICAL MEMORY MANAGER",
                       "Initialized: zones=%lu total=%lu MB, free=%lu MB", m_zone_count,
                       m_total_memory / (1024 * 1024), m_free_memory / (1024 * 1024));
}

PhysicalZone* PhysicalMemoryManager::find_zone_for_paddr(uintptr_t phys) {
  for (size_t i = 0; i < m_zone_count; ++i) {
    auto& z = m_zones[i].zone;
    if (phys >= z.base() && phys < z.base() + z.length()) {
      return &m_zones[i];
    }
  }

  fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "No zone found for phys=%p", phys);

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

  fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "No zones available");
  return nullptr;
}

uintptr_t PhysicalMemoryManager::alloc_page(ZoneType preferred, uint32_t preferred_node) {
  assert(m_is_initialized);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = select_zone(preferred, preferred_node);
  if (!pz) {
    fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "Alloc_page: No zone available");
    return 0;
  }

  ssize_t frame = pz->bitmap.alloc();
  if (frame >= 0) {
    uintptr_t phys = pz->zone.base() + (frame * FRAME_SIZE);

    m_free_memory -= FRAME_SIZE;
    return phys;
  }

  fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "Bitmap exhausted, falling back to buddy");

  void* ptr = pz->buddy.alloc(0);
  if (!ptr)
    return 0;

  m_free_memory -= FRAME_SIZE;

  return reinterpret_cast<uintptr_t>(ptr);
}

void PhysicalMemoryManager::free_page(uintptr_t phys) {
  assert(m_is_initialized);
  assert((phys % FRAME_SIZE) == 0);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz) {
    fk::algorithms::kerror("PHYSICAL MEMORY MANAGER", "free_page: no zone found for address %p",
                           phys);
    return;
  }

  uintptr_t base = pz->zone.base();
  uintptr_t end = base + pz->zone.length();

  if (phys >= base && phys < end) {
    size_t frame = (phys - base) / FRAME_SIZE;
    pz->bitmap.clear(frame);
  } else {
    pz->buddy.free(reinterpret_cast<void*>(phys), 0);
  }

  m_free_memory += FRAME_SIZE;
}

uintptr_t PhysicalMemoryManager::alloc_contiguous(size_t order, ZoneType preferred,
                                                  uint32_t preferred_node) {
  assert(m_is_initialized);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = select_zone(preferred, preferred_node);
  if (!pz) {
    fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "alloc_contiguous: No zone available");
    return 0;
  }

  void* block = pz->buddy.alloc(order);
  if (!block) {
    fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER",
                          "alloc_contiguous: Buddy allocation failed for order "
                          "%lu in zone type %d",
                          order, (int)pz->zone.type());
    return 0;
  }

  uintptr_t phys = reinterpret_cast<uintptr_t>(block);
  m_free_memory -= (FRAME_SIZE << order);
  return phys;
}

void PhysicalMemoryManager::free_contiguous(uintptr_t phys, size_t order) {
  assert(m_is_initialized);
  assert((phys % FRAME_SIZE) == 0);
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  PhysicalZone* pz = find_zone_for_paddr(phys);
  if (!pz) {
    fk::algorithms::kerror("PHYSICAL MEMORY MANAGER",
                           "free_contiguous: no zone found for address %p", phys);
    return;
  }

  pz->buddy.free(reinterpret_cast<void*>(phys), order);
  m_free_memory += (FRAME_SIZE << order);
}