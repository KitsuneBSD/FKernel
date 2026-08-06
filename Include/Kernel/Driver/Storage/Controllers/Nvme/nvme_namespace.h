#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct NvmeNamespace {
  uint32_t nsid{0};
  uint64_t size_blocks{0};
  uint32_t block_size{512};
  bool active{false};
};

} // namespace fkernel
