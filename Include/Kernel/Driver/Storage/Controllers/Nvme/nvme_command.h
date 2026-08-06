#pragma once

#include <LibFK/Types/types.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_completion.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_completion_queue.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_submission_queue.h>

namespace fkernel {

struct NvmeCommand {
  uint32_t cdw0;
  uint32_t nsid;
  uint32_t rsvd2[2]; // CDW2/CDW3 (reserved per NVMe 1.4 SQE layout)
  uint64_t mptr;     // Metadata Pointer (offset 16)
  uint64_t prp1;
  uint64_t prp2;
  uint32_t cdw10;
  uint32_t cdw11;
  uint32_t cdw12;
  uint32_t cdw13;
  uint32_t cdw14;
  uint32_t cdw15;
} __attribute__((packed));

} // namespace fkernel
