#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct NvmeCompletionQueue {
  void* cq_memory = nullptr;
  uint16_t cq_size = 0;
  uint16_t cq_head = 0;
  uint16_t cq_phase = true;
  volatile uint32_t* cq_head_db = nullptr;
  uint32_t cq_phys_addr = 0;
};

} // namespace fkernel
