#pragma once

#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class NvmeQueueUtilities {
public:
  static fk::core::Result<void, fk::core::Error>
  create_submission_queue(NvmeSubmissionQueue& sq, uint16_t size, uint16_t cq_id,
                          volatile uint8_t* controller_regs);

  static fk::core::Result<void, fk::core::Error>
  create_completion_queue(NvmeCompletionQueue& cq, uint16_t size,
                          volatile uint8_t* controller_regs);

  static uint32_t process_completions(NvmeCompletionQueue& cq);

private:
  static fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
};

} // namespace fkernel
