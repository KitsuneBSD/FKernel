#pragma once

#include <LibFK/Types/types.h>

namespace boot {

struct MemoryMapEntry {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  bool is_available;
};

} // namespace boot
