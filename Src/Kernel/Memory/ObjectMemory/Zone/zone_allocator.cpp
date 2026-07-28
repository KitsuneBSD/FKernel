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
  if (!m_initialized) {
    fk::algorithms::kwarn("ZONE", "base() called on uninitialized zone");
    return 0;
  }
  return m_base;
}

size_t Zone::length() const {
  if (!m_initialized) {
    fk::algorithms::kwarn("ZONE", "length() called on uninitialized zone");
    return 0;
  }
  return m_length;
}

size_t Zone::frame_count() const {
  if (!m_initialized) {
    fk::algorithms::kwarn("ZONE", "frame_count() called on uninitialized zone");
    return 0;
  }
  return m_frame_count;
}

ZoneType Zone::type() const {
  if (!m_initialized) {
    fk::algorithms::kwarn("ZONE", "type() called on uninitialized zone");
    return ZoneType::NORMAL;
  }
  return m_type;
}
