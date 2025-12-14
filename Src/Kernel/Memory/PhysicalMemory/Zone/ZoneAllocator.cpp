#include <Kernel/Memory/PhysicalMemory/Zone/ZoneAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneDefs.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneType.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Types/types.h>

void Zone::populate_zone(uintptr_t base, size_t length, ZoneType type) {
  assert((base % FRAME_SIZE) == 0);
  assert((length % FRAME_SIZE) == 0);
  assert((length / FRAME_SIZE) > 0);

  m_base        = base;
  m_length      = length;
  m_frame_count = length / FRAME_SIZE;
  m_type        = type;
  m_initialized = true;

  fk::algorithms::kdebug(
      "ZONE",
      "Zone populate: base=%p size=%lu frames=%lu type=%d",
      m_base,
      m_length,
      m_frame_count,
      static_cast<int>(m_type));
}

uintptr_t Zone::base() const {
  assert(m_initialized);
  return m_base;
}

size_t Zone::length() const {
  assert(m_initialized);
  return m_length;
}

size_t Zone::frame_count() const {
  assert(m_initialized);
  return m_frame_count;
}

ZoneType Zone::type() const {
  assert(m_initialized);
  return m_type;
}
