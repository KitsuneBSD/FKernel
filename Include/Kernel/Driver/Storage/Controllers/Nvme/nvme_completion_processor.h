#pragma once

#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_manager.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_async_operation.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command.h>
#include <Kernel/Driver/Async/async_io.h>
#include <LibFK/Container/Sequence/array.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class NvmeCompletionProcessor {
private:
  NvmeCompletionQueue& m_queue;
  NvmeCompletionQueue& m_admin_queue;
  fk::containers::array<NvmeAsyncOperation*, 256>& m_pending_operations;

public:
  NvmeCompletionProcessor(NvmeCompletionQueue& queue, NvmeCompletionQueue& admin_queue,
                          fk::containers::array<NvmeAsyncOperation*, 256>& pending)
      : m_queue(queue), m_admin_queue(admin_queue), m_pending_operations(pending) {}

  void process_admin_completions();
  void process_io_completions();

private:
  void advance_queue_head(NvmeCompletionQueue& queue);
  bool is_new_completion(const NvmeCompletion& completion, const NvmeCompletionQueue& queue);
  void handle_completion(uint16_t command_id, uint16_t status);
};

} // namespace fkernel
