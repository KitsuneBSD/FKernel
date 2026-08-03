#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct NvmeQueueDescriptor {
  void* memory{nullptr};
  uint32_t size{0};
  uint16_t head{0};
  uint16_t tail{0};
  bool phase{true};
  uint32_t phys_addr{0};
  volatile uint32_t* head_db{nullptr};
  void* cq_memory{nullptr};
  uint32_t cq_size{0};
  uint16_t cq_head{0};
  bool cq_phase{true};
  uint32_t cq_phys_addr{0};
  volatile uint32_t* cq_head_db{nullptr};
};

} // namespace fkernel
