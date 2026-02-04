#include <Kernel/Driver/Storage/Nvme/NvmeCommandBuilder.h>
#include <Kernel/Driver/Storage/Nvme/NvmeQueueManager.h>

namespace fkernel {

NvmeCommandBuilder::NvmeCommandBuilder(NvmeQueueManager& queue_manager)
    : m_queue_manager(queue_manager), m_pending_operations(256), m_command_id_usage(256, false) {}

NvmeCommandBuilder::NvmeCommand NvmeCommandBuilder::build_read_command(uint64_t lba,
                                                                       uint32_t block_count,
                                                                       uint64_t prp1,
                                                                       uint64_t prp2) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x02; // NVME_CMD_READ
  cmd.nsid = 1;    // Assuming namespace 1
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = (uint32_t)lba;         // Starting LBA
  cmd.cdw11 = (uint32_t)(lba >> 32); // Starting LBA high
  cmd.cdw12 = block_count - 1;       // Number of blocks (0-based)
  return cmd;
}

NvmeCommandBuilder::NvmeCommand NvmeCommandBuilder::build_write_command(uint64_t lba,
                                                                        uint32_t block_count,
                                                                        uint64_t prp1,
                                                                        uint64_t prp2) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x01; // NVME_CMD_WRITE
  cmd.nsid = 1;    // Assuming namespace 1
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = (uint32_t)lba;         // Starting LBA
  cmd.cdw11 = (uint32_t)(lba >> 32); // Starting LBA high
  cmd.cdw12 = block_count - 1;       // Number of blocks (0-based)
  return cmd;
}

NvmeCommandBuilder::NvmeCommand NvmeCommandBuilder::build_flush_command() {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x00; // NVME_CMD_FLUSH
  cmd.nsid = 1;    // Assuming namespace 1
  return cmd;
}

NvmeCommandBuilder::NvmeCommand NvmeCommandBuilder::build_identify_command(uint32_t nsid) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x06; // NVME_ADMIN_IDENTIFY
  cmd.nsid = nsid;
  return cmd;
}

fk::core::Result<uint16_t, fk::core::Error>
NvmeCommandBuilder::submit_command(const NvmeCommand& command, uint32_t queue_id) {
  uint16_t command_id = find_free_command_id();
  if (command_id == 0xFFFF) {
    return fk::core::Error::DeviceBusy;
  }

  mark_command_id_used(command_id);

  // Submit command to queue manager
  auto submit_result = m_queue_manager.submit_command(&command, queue_id);
  if (submit_result.is_error()) {
    mark_command_id_free(command_id);
    return submit_result.error();
  }

  return command_id;
}

fk::core::Result > NvmeAsyncOperation*,
    fk::core::Error > NvmeCommandBuilder::submit_async_read(uint64_t start_lba,
                                                            uint32_t block_count, uint8_t* buffer) {
  uint16_t command_id = find_free_command_id();
  if (command_id == 0xFFFF) {
    return fk::core::Error::DeviceBusy;
  }

  mark_command_id_used(command_id);

  // Create async operation
  auto operation = new NvmeAsyncOperation(command_id, start_lba, block_count, buffer, false);
  if (!operation) {
    mark_command_id_free(command_id);
    return fk::core::Error::OutOfMemory;
  }

  m_pending_operations[command_id] = operation;

  // Setup PRP list
  uint64_t prp1 = MemoryManager::the().translate(reinterpret_cast<uintptr_t>(buffer));
  uint64_t prp2 = 0;

  if (block_count * 512 > 4096) {
    fk::algorithms::kerror("NVMe-CB", "Large transfers not yet supported");
    mark_command_id_free(command_id);
    return fk::core::Error::NotImplemented;
  }

  // Build read command
  NvmeCommand cmd = build_read_command(start_lba, block_count, prp1, prp2);
  cmd.cdw0 |= (command_id & 0xFFFF) < >> 16;

  // Submit command
  auto submit_result = submit_command(cmd, 1);
  if (submit_result.is_error()) {
    mark_command_id_free(command_id);
    return submit_result.error();
  }

  fk::algorithms::klog("NVMe-CB", "Submitted async read: command_id=%u, lba=%lu, count=%u",
                       command_id, start_lba, block_count);

  return operation;
}

fk::core::Result<uint16_t, fk::core::Error>
NvmeCommandBuilder::submit_async_write(uint64_t start_lba, uint32_t block_count,
                                       const uint8_t* buffer) {
  uint16_t command_id = find_free_command_id();
  if (command_id == 0xFFFF) {
    return fk::core::Error::DeviceBusy;
  }

  mark_command_id_used(command_id);

  // Create async operation
  auto operation = new NvmeAsyncOperation(command_id, start_lba, block_count,
                                          const_cast<uint8_t*>(buffer), true);
  if (!operation) {
    mark_command_id_free(command_id);
    return fk::core::Error::OutOfMemory;
  }

  m_pending_operations[command_id] = operation;

  // Setup PRP list
  uint64_t prp1 = MemoryManager::the().translate(reinterpret_cast<uintptr_t>(buffer));
  uint64_t prp2 = 0;

  if (block_count * 512 > 4096) {
    fk::algorithms::kerror("NVMe-CB", "Large transfers not yet supported");
    mark_command_id_free(command_id);
    return fk::core::Error::NotImplemented;
  }

  // Build write command
  NvmeCommand cmd = build_write_command(start_lba, block_count, prp1, prp2);
  cmd.cdw0 |= (command_id & 0xFFFF) < >> 16;

  // Submit command
  auto submit_result = submit_command(cmd, 1);
  if (submit_result.is_error()) {
    mark_command_id_free(command_id);
    return submit_result.error();
  }

  fk::algorithms::klog("NVMe-CB", "Submitted async write: command_id=%u, lba=%lu, count=%u",
                       command_id, start_lba, block_count);

  return command_id;
}

fk::core::Result<void, fk::core::Error>
NvmeCommandBuilder::wait_for_completion(uint16_t command_id, uint32_t timeout_ms) {
  if (command_id >= m_pending_operations.size()) {
    return fk::core::Error::InvalidParameter;
  }

  auto& operation = m_pending_operations[command_id];
  if (!operation) {
    return fk::core::Error::InvalidParameter;
  }

  // Wait for completion
  IoCompletionStatus status = operation->wait_for_completion(timeout_ms);

  if (status == IoCompletionStatus::Success) {
    return {};
  } else if (status == IoCompletionStatus::Timeout) {
    return fk::core::Error::Timeout;
  } else {
    return fk::core::Error::DeviceError;
  }
}

uint16_t NvmeCommandBuilder::find_free_command_id() {
  for (uint16_t id = 1; id < m_command_id_usage.size(); ++id) {
    if (!m_command_id_usage[id]) {
      return id;
    }
  }
  return 0xFFFF; // No free command IDs
}

void NvmeCommandBuilder::mark_command_id_used(uint16_t command_id) {
  if (command_id < m_command_id_usage.size()) {
    m_command_id_usage[command_id] = true;
  }
}

void NvmeCommandBuilder::mark_command_id_free(uint16_t command_id) {
  if (command_id < m_command_id_usage.size()) {
    m_command_id_usage[command_id] = false;
    m_pending_operations[command_id] = nullptr;
  }
}

} // namespace fkernel