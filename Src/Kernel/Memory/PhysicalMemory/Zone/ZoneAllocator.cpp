#include <Kernel/Memory/PhysicalMemory/Zone/ZoneAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneDefs.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneType.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Types/types.h>

void Zone::mark_used(size_t index, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    m_bitmap.set(index + i, true);
  }

  fk::algorithms::kdebug("ZONE", "Marked [%lu .. %lu] as used", index,
                         index + count - 1);
}

void Zone::mark_free(size_t index, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    m_bitmap.set(index + i, false);
  }

  fk::algorithms::kdebug("ZONE", "Marked [%lu .. %lu] as free", index,
                         index + count - 1);
}

uintptr_t Zone::alloc_frame() {
  for (size_t i = 0; i < m_frame_count; ++i) {
    if (!m_bitmap.get(i)) {
      m_bitmap.set(i, true);
      uintptr_t addr = m_base + i * FRAME_SIZE;

      fk::algorithms::kdebug("ZONE", "Allocated frame @ %p (index %lu)", addr,
                             i);

      return addr;
    }
  }

  fk::algorithms::kdebug("ZONE", "No free frames available");
  return 0;
}

void Zone::free_frame(uintptr_t addr) {
  assert(addr >= m_base && addr < m_base + m_length);

  size_t index = (addr - m_base) / FRAME_SIZE;
  m_bitmap.set(index, false);

  fk::algorithms::kdebug("ZONE", "Freed frame @ %p (index %lu)", addr, index);
}

bool Zone::is_free(size_t index) const {
  assert(index < m_frame_count);

  return !m_bitmap.get(index);
}
