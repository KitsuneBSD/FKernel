#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct DirectoryEntry {
  char name[256];
  uint32_t type;
};

} // namespace fkernel
