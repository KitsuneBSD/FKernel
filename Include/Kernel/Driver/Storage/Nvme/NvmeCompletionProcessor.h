#pragma once

#include <Kernel/Driver/Storage/Nvme/NvmeCommandBuilder.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

class NvmeCompletionProcessor {
public:
  NvmeCompletionProcessor(NvmeCommandBuilder& command_builder);

  // Process completions from admin queue
  fk::core::Result<void, fk::core::Error> process_admin_completions();

  // Process completions from I/O queues
  fk::core::Result<void, fk::core::Error> process_io_completions(uint32_t queue_id);

  // Handle interrupts
  void handle_interrupt();

  // Register interrupt handler
  void register_interrupt_handler();

private:
  void ring_completion_doorbell(const NvmeQueueManager::Queue& queue);

  NvmeCommandBuilder& m_command_builder;
  bool m_interrupts_enabled = false;
};

} // namespace fkernel