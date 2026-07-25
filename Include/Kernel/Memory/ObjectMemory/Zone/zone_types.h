/**
 * @file zone_types.h
 * @brief Zone types and classification logic.
 */

#pragma once 

#include <Kernel/Memory/ObjectMemory/Zone/zone_defs.h>

/**
 * @enum ZoneType
 * @brief Classifies memory based on its physical address.
 */
enum class ZoneType {
    DMA,    ///< Memory below 16MB.
    NORMAL, ///< Memory between 16MB and 4GB.
    HIGH,   ///< Memory above 4GB.
};

/**
 * @brief Classifies a physical address into a ZoneType.
 * @param base The physical base address.
 * @return The corresponding ZoneType.
 */
static inline ZoneType classify_zone(uintptr_t base) {
  if (base < DMA_LIMIT)
    return ZoneType::DMA;
  if (base >= HIGH_LIMIT)
    return ZoneType::HIGH;
  return ZoneType::NORMAL;
}

/**
 * @brief Returns the upper address limit for a given zone type.
 * @param type The zone type.
 * @return The physical address limit.
 */
static inline uintptr_t zone_limit(ZoneType type) {
  switch (type) {
    case ZoneType::DMA:    return DMA_LIMIT;
    case ZoneType::NORMAL: return HIGH_LIMIT;
    case ZoneType::HIGH:   return UINTPTR_MAX;
  }
  return UINTPTR_MAX;
}