#pragma once

#include <LibFK/Types/types.h>

namespace boot {

struct ModuleInfo {
  uint64_t start;
  uint64_t end;
  const char* cmdline;
};

} // namespace boot
