#pragma once

#include <Kernel/Memory/ObjectMemory/Zone/zone_defs.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_types.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Types/types.h>

/**
 * @class Zone
 * @brief Represents a physical memory range with a specific type (DMA, NORMAL, HIGH).
 */
class Zone {
private:
  uintptr_t m_base{0};
  size_t    m_length{0};
  size_t    m_frame_count{0};
  ZoneType  m_type{};
  bool      m_initialized{false};


public:
  Zone() = default;

  /**
   * @brief Constructs a zone with specified boundaries and type.
   * @param base Physical base address (must be frame-aligned).
   * @param length Length in bytes (must be frame-aligned).
   * @param type The zone classification.
   */
  Zone(uintptr_t base, size_t length, ZoneType type) : m_base(base), m_length(length), m_frame_count(length / FRAME_SIZE), m_type(type) {
    assert((m_base % FRAME_SIZE) == 0);

    assert((m_length % FRAME_SIZE) == 0);

    assert(m_frame_count > 0);
  };

  /**
   * @brief Populates an uninitialized zone with new parameters.
   */
  void populate_zone(uintptr_t base, size_t length, ZoneType type); 

  /** @return The physical base address of the zone. */
  uintptr_t base() const;

  /** @return The length of the zone in bytes. */
  size_t length() const;

  /** @return The total number of 4KB frames in the zone. */
  size_t frame_count() const;

  /** @return The type of the zone. */
  ZoneType type() const;

};
