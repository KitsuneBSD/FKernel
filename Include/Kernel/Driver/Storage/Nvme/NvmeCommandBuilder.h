#pragma once

#include <Kernel/Driver/Storage/Nvme/NvmeQueueManager.h>
#include <Kernel/Driver/async_io.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

class NvmeCommandBuilder {
public:
  struct NvmeCommand {
    uint32_t cdw0 = 0;
    uint32_t nsid = 0;
    uint64_t prp1 = 0;
    uint64_t prp2 = 0;
    uint32_t cdw10 = 0;
    uint32_t cdw11 = 0;
    uint32_t cdw12 = 0;
    uint32_t cdw13 = 0;
    uint32_t cdw14 = 0;
    uint32_t cdw15 = 0;
  };

  NvmeCommandBuilder(NvmeQueueManager& queue_manager);

  // Command builders
  NvmeCommand build_read_command(uint64_t lba, uint32_t block_count, uint64_t prp1, uint64_t prp2);
  NvmeCommand build_write_command(uint64_t lba, uint32_t block_count, uint64_t prp1, uint64_t prp2);
  NvmeCommand build_flush_command();
  NvmeCommand build_identify_command(uint32_t nsid);

  // Command submission
  fk::core::Result<uint16_t, fk::core::Error> submit_command(const NvmeCommand& command,
                                                             uint32_t queue_id);

  // Async operation management
  fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
  submit_async_read(uint64_t start_lba, uint32_t block_count, uint8_t* buffer);
  fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
  submit_async_write(uint64_t start_lba, uint32_t block_count, const uint8_t* buffer);

  // Define NvmeAsyncOperation struct
  struct NvmeAsyncOperation {
    uint16_t command_id;
    uint32_t queue_id;
    bool completed;
    fk::core::Result<void, fk::core::Error> result;
  };

  // Completion handling
  fk::core::Result<void, fk::core::Error> wait_for_completion(uint16_t command_id,
                                                              uint32_t timeout_ms);

  // Queue manager access
  NvmeQueueManager& queue_manager() { return m_queue_manager; }

private:
  uint16_t find_free_command_id();
  void mark_command_id_used(uint16_t command_id);
  void mark_command_id_free(uint16_t command_id);
  fk::core::Result<void, fk::core::Error> complete_operation(uint16_t command_id,
                                                             IoCompletionStatus status);
  fk::containers::Vector<NvmeAsyncOperation*> m_pending_operations;
  fk::containers::Vector<bool> m_command_id_usage;
  fk::containers::Vector<NvmeAsyncOperation*> m_pending_operations;
  fk::containers::Vector<bool> m_command_id_usage;
};

} // namespace fkernel