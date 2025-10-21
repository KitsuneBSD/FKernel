#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

struct MemoryMapEntry {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
};

struct MemoryMapView {
  MemoryMapEntry const *entries;
  size_t count;
};
