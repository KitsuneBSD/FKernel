#pragma once

#include <Kernel/Memory/ObjectMemory/Zone/zone_defs.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_types.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Types/types.h>

class Zone {
private:
  uintptr_t m_base{0};
  size_t    m_length{0};
  size_t    m_frame_count{0};
  ZoneType  m_type{};
  bool      m_initialized{false};


public:
  Zone() = default;

  Zone(uintptr_t base, size_t length, ZoneType type) : m_base(base), m_length(length), m_frame_count(length / FRAME_SIZE), m_type(type) {
    assert((m_base % FRAME_SIZE) == 0);

    assert((m_length % FRAME_SIZE) == 0);

    assert(m_frame_count > 0);
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "ZONE",
        "Zone created: base=%p size=%lu frames=%lu type=%d",
        m_base,
        m_length,
        m_frame_count,
        static_cast<int>(m_type));
    */
  };

  void populate_zone(uintptr_t base, size_t length, ZoneType type); 

  uintptr_t base() const;
  size_t length() const;
  size_t frame_count() const;
  ZoneType type() const;

};
