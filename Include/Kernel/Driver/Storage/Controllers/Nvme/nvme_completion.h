#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct NvmeCompletion {
  uint32_t cdw0;
  uint32_t rsvd;
  uint16_t sq_head;
  uint16_t sq_id;
  uint16_t command_id;
  uint16_t status;
};

} // namespace fkernel
