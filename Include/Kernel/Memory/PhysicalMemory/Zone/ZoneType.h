#pragma once 



enum class ZoneType {
    DMA, // <= 16MB
    NORMAL, // <= Common Random Access Memory
    HIGH, // <= Random Access Memory after 4GB
};

static inline ZoneType classify_zone(uintptr_t base) {
  if (base < DMA_LIMIT)
    return ZoneType::DMA;
  if (base >= HIGH_LIMIT)
    return ZoneType::HIGH;
  return ZoneType::NORMAL;
}

static inline uintptr_t zone_limit(ZoneType type) {
  switch (type) {
    case ZoneType::DMA:    return DMA_LIMIT;
    case ZoneType::NORMAL: return HIGH_LIMIT;
    case ZoneType::HIGH:   return UINTPTR_MAX;
  }
  return UINTPTR_MAX;
}