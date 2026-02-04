#include <Kernel/Driver/Storage/Nvme/NvmeCommandBuilder.h>
#include <Kernel/Driver/Storage/Nvme/NvmeCompletionProcessor.h>

namespace fkernel {

NvmeCompletionProcessor::NvmeCompletionProcessor(NvmeCommandBuilder& command_builder)
    : m_command_builder(command_builder) {}

fk::core::Result<void, fk::core::Error> NvmeCompletionProcessor::process_admin_completions() {
  // Process admin completions from queue manager
  auto& admin_queue = m_command_builder.queue_manager().admin_queue();

  volatile NvmeQueueManager::NvmeCompletion* cq =
      reinterpret_cast<volatile NvmeQueueManager::NvmeCompletion*>(admin_queue.cq_memory);

  while (true) {
    NvmeQueueManager::NvmeCompletion completion;
    memcpy(&completion, &cq[admin_queue.cq_head], sizeof(NvmeQueueManager::NvmeCompletion));

    if ((completion.status & 0x1) != (admin_queue.cq_phase ? 1 : 0)) {
      break;
    }

    admin_queue.cq_head = (admin_queue.cq_head + 1) % admin_queue.cq_size;
    if (admin_queue.cq_head == 0) {
      admin_queue.cq_phase = !admin_queue.cq_phase;
    }
  }

  ring_completion_doorbell(admin_queue);
  return {};
}

fk::core::Result<void, fk::core::Error>
NvmeCompletionProcessor::process_io_completions(uint32_t queue_id) {
  if (queue_id >= 8) {
    return fk::core::Error::InvalidParameter;
  }

  auto& io_queue = m_command_builder.queue_manager().io_queue(queue_id);

  volatile NvmeQueueManager::NvmeCompletion* cq =
      reinterpret_cast<volatile NvmeQueueManager::NvmeCompletion*>(io_queue.cq_memory);

  while (true) {
    NvmeQueueManager::NvmeCompletion completion;
    memcpy(&completion, &cq[io_queue.cq_head], sizeof(NvmeQueueManager::NvmeCompletion));

    if ((completion.status & 0x1) != (io_queue.cq_phase ? 1 : 0)) {
      break;
    }

    uint16_t command_id = completion.command_id;
    IoCompletionStatus status = IoCompletionStatus::Success;

    if ((completion.status >> 1) != 0) {
      fk::algorithms::kerror("NVMe-CP", "Command %u failed with status 0x%x", command_id,
                             completion.status >> 1);
      status = IoCompletionStatus::Error;
    }

    // Complete operation
    auto operation_res = m_command_builder.complete_operation(command_id, status);
    if (operation_res.is_error()) {
      fk::algorithms::kerror("NVMe-CP", "Failed to complete operation %u: %d", command_id,
                             (int)operation_res.error());
    }

    io_queue.cq_head = (io_queue.cq_head + 1) % io_queue.cq_size;
    if (io_queue.cq_head == 0) {
      io_queue.cq_phase = !io_queue.cq_phase;
    }
  }

  ring_completion_doorbell(io_queue);
  return {};
}

void NvmeCompletionProcessor::handle_interrupt() {
  if (!m_interrupts_enabled) {
    return;
  }

  // Clear interrupt
  // In a real implementation, we would write to the interrupt clear register

  // Process completions
  process_admin_completions();

  for (uint32_t i = 0; i < 8; ++i) {
    if (m_command_builder.queue_manager().io_queue(i).memory) {
      process_io_completions(i);
    }
  }
}

void NvmeCompletionProcessor::register_interrupt_handler() {
  // Register with interrupt controller
  // In a real implementation, we would register the interrupt handler
  m_interrupts_enabled = true;
  fk::algorithms::klog("NVMe-CP", "Interrupt handler registered");
}

void NvmeCompletionProcessor::ring_completion_doorbell(const NvmeQueueManager::Queue& queue) {
  // Ring completion queue doorbell
  // In a real implementation, we would write to the doorbell register
  // For now, we just log
  fk::algorithms::klog("NVMe-CP", "Ringing completion doorbell for queue head %u", queue.cq_head);
}

} // namespace fkernel