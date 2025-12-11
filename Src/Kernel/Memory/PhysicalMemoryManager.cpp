#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyOrder.h>
#include <Kernel/Memory/PhysicalMemoryManager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/new.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (m_is_initialized)
    return;

  fk::algorithms::klog("PHYSICAL MEMORY MANAGER",
                       "Initializing physical memory manager");

  for (const auto &entry : *mmap) {
    if (!multiboot2::is_available(entry.type))
      continue;

    uintptr_t start_addr = entry.base_addr;
    uintptr_t end_addr = entry.base_addr + entry.length;

    uintptr_t length_u = end_addr - start_addr;
    size_t length = static_cast<size_t>(length_u);

    m_total_memory += length;
    m_free_memory += length;

    m_buddy.add_range(start_addr, length);

    register_zone(static_cast<uint64_t>(start_addr),
                  static_cast<uint64_t>(length), ZoneType::NORMAL);

    fk::algorithms::kdebug("PHYSICAL MEMORY MANAGER",
                           "Usable range: [%p - %p] (%lu KB)", start_addr,
                           end_addr, length / 1024);
  }

  m_is_initialized = true;
}

void PhysicalMemoryManager::register_zone(uintptr_t base, size_t length,
                                          ZoneType type) {
  assert(m_zone_count < MAX_ZONES);

  new (&m_zones[m_zone_count]) Zone(base, length, type);

  m_zone_count++;

  fk::algorithms::kdebug("PMM", "Zone registered: %p size %lu KB type %d", base,
                         length / 1024, (int)type);
}

Zone *PhysicalMemoryManager::find_zone_for_addr(uintptr_t addr) {
  for (size_t i = 0; i < m_zone_count; i++) {
    auto *z = m_zones[i];
    if (addr >= z->base() && addr < z->base() + z->length())
      return z;
  }
  return nullptr;
}

Zone *PhysicalMemoryManager::select_zone(ZoneType preferred) {
  for (size_t i = 0; i < m_zone_count; i++) {
    if (m_zones[i]->type() == preferred)
      return m_zones[i];
  }
  return nullptr;
}

uintptr_t PhysicalMemoryManager::alloc_page(ZoneType preferred) {
  Zone *zone = select_zone(preferred);

  uintptr_t addr = 0;

  if (zone)
    addr = zone->alloc_frame();

  if (!addr)
    addr = reinterpret_cast<uintptr_t>(m_buddy.alloc(0)); // order 0 = 4K

  if (addr)
    m_free_memory -= 4096;

  return addr;
}

void PhysicalMemoryManager::free_page(uintptr_t addr) {
  Zone *zone = find_zone_for_addr(addr);

  if (zone) {
    zone->free_frame(addr);
  } else {
    auto buddy_addr = reinterpret_cast<void *>(addr);
    m_buddy.free(buddy_addr, 0);
  }

  m_free_memory += 4096;
}

uintptr_t PhysicalMemoryManager::alloc_contiguous(size_t order) {
  void *buddy_addr = m_buddy.alloc(order);
  auto addr = reinterpret_cast<uintptr_t>(buddy_addr);
  if (addr)
    m_free_memory -= (1ull << order) * 4096;
  return addr;
}

void PhysicalMemoryManager::free_contiguous(uintptr_t addr, size_t order) {
  auto buddy_addr = reinterpret_cast<void *>(addr);
  m_buddy.free(buddy_addr, order);
  m_free_memory += (1ull << order) * 4096;
}
