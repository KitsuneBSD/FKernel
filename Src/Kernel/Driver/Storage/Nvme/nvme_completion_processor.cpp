#include <Kernel/Driver/Storage/Nvme/nvme_completion_processor.h>
#include <LibC/string.h>

namespace fkernel {

void NvmeCompletionProcessor::process_admin_completions() {
  auto* cq = reinterpret_cast<volatile NvmeCompletion*>(m_admin_queue.cq_memory);

  while (true) {
    NvmeCompletion completion;
    memcpy(&completion, const_cast<const NvmeCompletion*>(&cq[m_admin_queue.cq_head]),
           sizeof(NvmeCompletion));

    if ((completion.status & 0x1) != (m_admin_queue.cq_phase ? 1 : 0))
      break;

    m_admin_queue.cq_head = (m_admin_queue.cq_head + 1) % m_admin_queue.cq_size;
    if (m_admin_queue.cq_head == 0)
      m_admin_queue.cq_phase = !m_admin_queue.cq_phase;
  }

  if (m_admin_queue.cq_head_db)
    *m_admin_queue.cq_head_db = m_admin_queue.cq_head;
}

void NvmeCompletionProcessor::process_io_completions() {
  for (uint32_t i = 0; i < 8; ++i) {
    if (!m_queue.cq_memory)
      continue;

    auto* cq = reinterpret_cast<volatile NvmeCompletion*>(m_queue.cq_memory);

    while (true) {
      NvmeCompletion completion;
      memcpy(&completion, const_cast<const NvmeCompletion*>(&cq[m_queue.cq_head]),
             sizeof(NvmeCompletion));

      if ((completion.status & 0x1) != (m_queue.cq_phase ? 1 : 0))
        break;

      uint16_t cmd_id = completion.command_id;
      IoCompletionStatus status = IoCompletionStatus::Success;

      if ((completion.status >> 1) != 0) {
        fk::algorithms::kerror("NVMe-INT", "Command %d failed: 0x%x", cmd_id,
                               completion.status >> 1);
        status = IoCompletionStatus::Error;
      }

      handle_completion(cmd_id, static_cast<uint16_t>(status));

      m_queue.cq_head = (m_queue.cq_head + 1) % m_queue.cq_size;
      if (m_queue.cq_head == 0)
        m_queue.cq_phase = !m_queue.cq_phase;
    }

    if (m_queue.cq_head_db)
      *m_queue.cq_head_db = m_queue.cq_head;
  }
}

void NvmeCompletionProcessor::advance_queue_head(NvmeCompletionQueue& queue) {
  queue.cq_head = (queue.cq_head + 1) % queue.cq_size;
  if (queue.cq_head == 0)
    queue.cq_phase = !queue.cq_phase;
}

bool NvmeCompletionProcessor::is_new_completion(const NvmeCompletion& completion,
                                                const NvmeCompletionQueue& queue) {
  return (completion.status & 0x1) == (queue.cq_phase ? 1 : 0);
}

void NvmeCompletionProcessor::handle_completion(uint16_t command_id, uint16_t status) {
  if (command_id >= m_pending_operations.size())
    return;

  auto& operation = m_pending_operations[command_id];
  if (!operation)
    return;

  if (status == 0) {
    operation->mark_success();
    return;
  }

  operation->mark_error();
  m_pending_operations[command_id] = nullptr;
}

} // namespace fkernel
