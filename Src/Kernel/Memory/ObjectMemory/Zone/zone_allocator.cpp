#include <Kernel/Memory/ObjectMemory/Zone/zone_allocator.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_defs.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_types.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
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
