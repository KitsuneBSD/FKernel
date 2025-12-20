#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_zone.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/new.h>

PhysicalZone* PhysicalMemoryManager::create_zone(
    uintptr_t base,
    size_t length,
    ZoneType type,
    uint64_t* bitmap_storage,
    size_t bitmap_bits)
{
    assert(m_zone_count < MAX_PHYSICAL_ZONES);
    assert((base % FRAME_SIZE) == 0);
    assert((length % FRAME_SIZE) == 0);
    assert(bitmap_storage != nullptr);
    assert(bitmap_bits > 0);

    PhysicalZone& pz = m_zones[m_zone_count];

    new (&pz.bitmap) fk::containers::Bitmap<uint64_t>(bitmap_storage, bitmap_bits);

    pz.zone.populate_zone(base, length, type);
    pz.buddy.add_range(base, length);

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY ZONE",
        "Zone created: base=%p size=%lu KB frames=%lu bitmap_bits=%lu type=%d",
        base,
        length / 1024,
        bitmap_bits,
        bitmap_bits,
        (int)type);
    */
    m_zone_count++;
    return &pz;
}


void PhysicalMemoryManager::process_range(
    uintptr_t base,
    uintptr_t end,
    uint64_t*& bitmap_cursor,
    size_t bitmap_words_remaining)
{
  while (base < end) {
    ZoneType type = classify_zone(base);
    uintptr_t limit = zone_limit(type);

    uintptr_t zone_end = end < limit ? end : limit;
    size_t length = zone_end - base;

    if (length == 0)
      break;

    size_t frames = length / FRAME_SIZE;
    size_t bitmap_bits = frames;
    size_t bitmap_words =
        (bitmap_bits + 63) / 64;

    assert(bitmap_words <= bitmap_words_remaining);

    create_zone(
      base,
      length,
      type, 
      bitmap_cursor,
      bitmap_bits
    );



    bitmap_cursor += bitmap_words;
    bitmap_words_remaining -= bitmap_words;

    base = zone_end;
  }
}

void PhysicalMemoryManager::initialize(
    const multiboot2::TagMemoryMap* mmap)
{
  assert(!m_is_initialized);

  fk::algorithms::klog("PHYSICAL MEMORY MANAGER", "Initializing Physical Memory Manager");

  extern uint64_t __pmm_bitmap_start[];
  extern uint64_t __pmm_bitmap_end[];

  uint64_t* bitmap_cursor = __pmm_bitmap_start;
  size_t bitmap_words_total =
      (__pmm_bitmap_end - __pmm_bitmap_start);
  size_t bitmap_words_remaining = bitmap_words_total;

  for (const auto& entry : *mmap) {
    if (!multiboot2::is_available(entry.type))
      continue;

    uintptr_t base = fk::utilities::align_up(entry.base_addr, FRAME_SIZE);
    uintptr_t end  = fk::utilities::align_down(entry.base_addr + entry.length, FRAME_SIZE);

    if (end <= base)
      continue;

    size_t usable = end - base;
    m_total_memory += usable;
    m_free_memory  += usable;

    process_range(
        base,
        end,
        bitmap_cursor,
        bitmap_words_remaining);
  }

  m_is_initialized = true;

  fk::algorithms::klog(
      "PHYSICAL MEMORY MANAGER",
      "Initialized: zones=%lu total=%lu MB",
      m_zone_count,
      m_total_memory / (1024 * 1024));
}

PhysicalZone* PhysicalMemoryManager::find_zone_for_paddr(uintptr_t phys)
{
    for (size_t i = 0; i < m_zone_count; ++i) {
        auto& z = m_zones[i].zone;
        if (phys >= z.base() &&
            phys <  z.base() + z.length()) {
            return &m_zones[i];
        }
    }

    fk::algorithms::kwarn(
        "PHYSICAL MEMORY MANAGER",
        "No zone found for phys=%p",
        phys);

    return nullptr;
}

PhysicalZone* PhysicalMemoryManager::select_zone(ZoneType preferred)
{
    for (size_t i = 0; i < m_zone_count; ++i) {
        if (m_zones[i].zone.type() == preferred) {
/*TODO: Apply this log when we work with LogLevel
            fk::algorithms::kdebug(
                "PHYSICAL MEMORY MANAGER",
                "Selected preferred zone type=%d",
                (int)preferred);
                */
            return &m_zones[i];
        }
    }

    for (size_t i = 0; i < m_zone_count; ++i) {
        if (m_zones[i].zone.type() == ZoneType::NORMAL) {
      /*TODO: Apply this log when we work with LogLevel
            fk::algorithms::kdebug(
                "PHYSICAL MEMORY MANAGER",
                "Preferred zone unavailable, falling back to NORMAL");
            */
            return &m_zones[i];
        }
    }

    if (m_zone_count > 0) {
    /*TODO: Apply this log when we work with LogLevel
        fk::algorithms::kdebug(
            "PHYSICAL MEMORY MANAGER",
            "Fallback to first available zone");
      */  
      return &m_zones[0];
    }

    fk::algorithms::kwarn(
        "PHYSICAL MEMORY MANAGER",
        "No zones available");
    return nullptr;
}

uintptr_t PhysicalMemoryManager::alloc_page(ZoneType preferred)
{
  assert(m_is_initialized);

  PhysicalZone* pz = select_zone(preferred);
  if (!pz) {
    fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "Alloc_page: No zone available");
    return 0;
  }

  ssize_t frame = pz->bitmap.alloc();
  if (frame >= 0) {
    uintptr_t phys =
        pz->zone.base() + (frame * FRAME_SIZE);

    m_free_memory -= FRAME_SIZE;
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY MANAGER",
        "Alloc_page(bitmap): phys=%p zone=%d",
        phys,
        (int)pz->zone.type());
*/
    return phys;
  }

  fk::algorithms::kwarn(
      "PHYSICAL MEMORY MANAGER",
      "Bitmap exhausted, falling back to buddy");

  void* ptr = pz->buddy.alloc(0);
  if (!ptr)
    return 0;

  m_free_memory -= FRAME_SIZE;

  return reinterpret_cast<uintptr_t>(ptr);
}

void PhysicalMemoryManager::free_page(uintptr_t phys)
{
  assert(m_is_initialized);
  assert((phys % FRAME_SIZE) == 0);

  PhysicalZone* pz = find_zone_for_paddr(phys);
  assert(pz != nullptr);

  uintptr_t base = pz->zone.base();
  uintptr_t end  = base + pz->zone.length();

  if (phys >= base && phys < end) {
    size_t frame = (phys - base) / FRAME_SIZE;
    pz->bitmap.clear(frame);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY MANAGER",
        "Free_page(bitmap): phys=%p frame=%lu",
        phys,
        frame);
*/
  } else {
    pz->buddy.free(reinterpret_cast<void*>(phys), 0);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY MANAGER",
        "Free_page(buddy): phys=%p",
        phys);
*/
  }

  m_free_memory += FRAME_SIZE;
}

uintptr_t PhysicalMemoryManager::alloc_contiguous(size_t order, ZoneType preferred) {
    assert(m_is_initialized);

    PhysicalZone* pz = select_zone(preferred);
    if (!pz) {
        fk::algorithms::kwarn("PHYSICAL MEMORY MANAGER", "alloc_contiguous: No zone available");
        return 0;
    }

    void* block = pz->buddy.alloc(order);
    if (!block) {
        fk::algorithms::kwarn(
            "PHYSICAL MEMORY MANAGER",
            "alloc_contiguous: Buddy allocation failed for order %lu in zone type %d",
            order,
            (int)pz->zone.type());
        return 0;
    }

    uintptr_t phys = reinterpret_cast<uintptr_t>(block);
    m_free_memory -= (FRAME_SIZE << order);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY MANAGER",
        "alloc_contiguous: phys=%p order=%lu size=%lu KB zone=%d",
        phys,
        order,
        (FRAME_SIZE << order) / 1024,
        (int)pz->zone.type());
*/
    return phys;
}

void PhysicalMemoryManager::free_contiguous(uintptr_t phys, size_t order) {
    assert(m_is_initialized);
    assert((phys % FRAME_SIZE) == 0);

    PhysicalZone* pz = find_zone_for_paddr(phys);
    assert(pz != nullptr);

    pz->buddy.free(reinterpret_cast<void*>(phys), order);
    m_free_memory += (FRAME_SIZE << order);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "PHYSICAL MEMORY MANAGER",
        "free_contiguous: phys=%p order=%lu size=%lu KB zone=%d",
        phys,
        order,
        (FRAME_SIZE << order) / 1024,
        (int)pz->zone.type());
  */
}
