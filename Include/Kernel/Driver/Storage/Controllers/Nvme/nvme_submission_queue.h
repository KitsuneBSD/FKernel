#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct NvmeSubmissionQueue {
  void* sq_memory = nullptr;
  uint16_t sq_size = 0;
  uint16_t sq_head = 0;
  uint16_t sq_tail = 0;
  volatile uint32_t* sq_tail_db = nullptr;
  bool sq_phase = true;
  uint32_t sq_phys_addr = 0;
};

} // namespace fkernel
