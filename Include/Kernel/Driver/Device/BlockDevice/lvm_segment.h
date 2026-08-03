#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct LvmSegment {
  uint64_t lv_start;
  uint64_t lv_end;
  size_t   pv_index;
  uint64_t pv_start;
};

} // namespace fkernel
