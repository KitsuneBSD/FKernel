#pragma once

#include "LibFK/Algorithms/log.h"
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneDefs.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneType.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Types/types.h>

class Zone {
private:
  uintptr_t m_base{0};
  size_t m_length{0};
  size_t m_frame_count{0};

  ZoneType m_type{ZoneType::NORMAL};
  fk::containers::Bitmap<uint64_t, BITMAP_SIZE> m_bitmap;

public:
  Zone() = default;
  Zone(uintptr_t base, size_t length, ZoneType type)
      : m_base(base), m_length(length), m_frame_count(length / FRAME_SIZE),
        m_type(type), m_bitmap(m_frame_count) {
    fk::algorithms::kdebug(
        "ZONE",
        "Create a new Zone instance with %lu base on %lu size on %d type", base,
        length, type);
    m_bitmap.clear_all();
  }

  uintptr_t base() const { return m_base; }
  size_t length() const { return m_length; }
  ZoneType type() const { return m_type; }
  size_t frame_count() const { return m_frame_count; }

  void mark_used(size_t index, size_t count = 1);
  void mark_free(size_t index, size_t count = 1);

  uintptr_t alloc_frame();
  void free_frame(uintptr_t addr);

  bool is_free(size_t index) const;
};
