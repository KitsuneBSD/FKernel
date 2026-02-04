#pragma once

#include <LibC/stdint.h>

namespace fkernel {

struct NvmeCommand {
  uint32_t cdw0;
  uint32_t nsid;
  uint64_t mptr;
  uint64_t prp1;
  uint64_t prp2;
  uint32_t cdw10;
  uint32_t cdw11;
  uint32_t cdw12;
  uint32_t cdw13;
  uint32_t cdw14;
  uint32_t cdw15;
};

struct NvmeCompletion {
  uint32_t cdw0;
  uint32_t rsvd;
  uint16_t sq_head;
  uint16_t sq_id;
  uint16_t command_id;
  uint16_t status;
};

struct NvmeSubmissionQueue {
  void* sq_memory = nullptr;
  uint16_t sq_size = 0;
  uint16_t sq_head = 0;
  uint16_t sq_tail = 0;
  volatile uint32_t* sq_tail_db = nullptr;
  bool sq_phase = true;
  uint32_t sq_phys_addr = 0;
};

struct NvmeCompletionQueue {
  void* cq_memory = nullptr;
  uint16_t cq_size = 0;
  uint16_t cq_head = 0;
  uint16_t cq_phase = true;
  volatile uint32_t* cq_head_db = nullptr;
  uint32_t cq_phys_addr = 0;
};

} // namespace fkernel
