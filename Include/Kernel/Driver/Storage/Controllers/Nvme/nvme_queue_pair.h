#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Driver/Async/dma_buffer.h>

namespace fkernel {

struct NvmeQueuePair {
  volatile uint32_t* sq_tail_db{nullptr};
  volatile uint32_t* cq_head_db{nullptr};
  DmaBuffer sq_buffer;
  DmaBuffer cq_buffer;
  void* sq_memory{nullptr};
  void* cq_memory{nullptr};
  uint16_t sq_size{0};
  uint16_t cq_size{0};
  uint16_t sq_tail{0};
  uint16_t cq_head{0};
  uint16_t cq_phase{1};
};

} // namespace fkernel
