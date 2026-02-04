#pragma once

#include <Kernel/Driver/Storage/Nvme/interrupt_driven_nvme.h>
#include <LibFK/Memory/own_ptr.h>

namespace fkernel {

/// @brief NVMe Command Builder
class NvmeCommandBuilder {
public:
  /// @brief Build a read command
  /// @param lba Starting logical block address
  /// @param block_count Number of blocks to read
  /// @param prp1 First PRP entry
  /// @param prp2 Second PRP entry (optional)
  /// @return NvmeCommand with read operation
  static NvmeCommand build_read_command(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                        uint64_t prp2 = 0) {
    NvmeCommand cmd = {};
    cmd.cdw0 = NVME_CMD_READ;
    cmd.prp1 = prp1;
    cmd.prp2 = prp2;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = block_count - 1;
    return cmd;
  }

  /// @brief Build a write command
  /// @param lba Starting logical block address
  /// @param block_count Number of blocks to write
  /// @param prp1 First PRP entry
  /// @param prp2 Second PRP entry (optional)
  /// @return NvmeCommand with write operation
  static NvmeCommand build_write_command(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                         uint64_t prp2 = 0) {
    NvmeCommand cmd = {};
    cmd.cdw0 = NVME_CMD_WRITE;
    cmd.prp1 = prp1;
    cmd.prp2 = prp2;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = block_count - 1;
    return cmd;
  }

  /// @brief Build a flush command
  /// @return NvmeCommand with flush operation
  static NvmeCommand build_flush_command() {
    NvmeCommand cmd = {};
    cmd.cdw0 = NVME_CMD_FLUSH;
    return cmd;
  }
};

/// @brief NVMe Queue Manager
class NvmeQueueManager {
public:
  /// @brief Create submission queue
  /// @param sq Submission queue structure to initialize
  /// @param size Number of entries in the queue
  /// @param cq_id Associated completion queue ID
  /// @return Result indicating success or failure
  static fk::core::Result<void, fk::core::Error>
  create_submission_queue(NvmeSubmissionQueue& sq, uint16_t size, uint16_t cq_id,
                          volatile uint8_t* controller_regs) {
    // Calculate queue attributes
    uint32_t aqa = ((size - 1) << 16) | (size - 1);
    *reinterpret_cast<volatile uint32_t*>(controller_regs + NVME_AQA) = aqa;

    // Allocate queue memory
    auto sq_res = allocate_dma_memory((size * sizeof(NvmeCommand)) + 4096);
    if (sq_res.is_error())
      return sq_res.error();
    uintptr_t sq_addr = sq_res.value();

    // Clear memory
    memset(reinterpret_cast<void*>(sq_addr), 0, size * sizeof(NvmeCommand));

    // Setup doorbell registers
    sq.sq_memory = reinterpret_cast<void*>(sq_addr);
    sq.sq_size = size;
    sq.sq_head = 0;
    sq.sq_tail = 0;
    sq.sq_phase = true;
    sq.sq_phys_addr = (uint32_t)sq_addr;
    sq.sq_tail_db = reinterpret_cast<volatile uint32_t*>(controller_regs + NVME_ASQ);

    return {};
  }

  /// @brief Create completion queue
  /// @param cq Completion queue structure to initialize
  /// @param size Number of entries in the queue
  /// @return Result indicating success or failure
  static fk::core::Result<void, fk::core::Error>
  create_completion_queue(NvmeCompletionQueue& cq, uint16_t size,
                          volatile uint8_t* controller_regs) {
    // Allocate queue memory
    auto cq_res = allocate_dma_memory((size * sizeof(NvmeCompletion)) + 4096);
    if (cq_res.is_error())
      return cq_res.error();
    uintptr_t cq_addr = cq_res.value();

    // Clear memory
    memset(reinterpret_cast<void*>(cq_addr), 0, size * sizeof(NvmeCompletion));

    // Setup doorbell registers
    cq.cq_memory = reinterpret_cast<void*>(cq_addr);
    cq.cq_size = size;
    cq.cq_head = 0;
    cq.cq_phase = true;
    cq.cq_phys_addr = (uint32_t)cq_addr;
    cq.cq_head_db = reinterpret_cast<volatile uint32_t*>(controller_regs + NVME_ACQ);

    return {};
  }

  /// @brief Process completions from a completion queue
  /// @param cq Completion queue to process
  /// @return Number of completions processed
  static uint32_t process_completions(NvmeCompletionQueue& cq) {
    volatile NvmeCompletion* cq_ptr = reinterpret_cast<volatile NvmeCompletion*>(cq.cq_memory);
    uint32_t completions = 0;

    while (true) {
      NvmeCompletion completion;
      memcpy(&completion, (void*)&cq_ptr[cq.cq_head], sizeof(NvmeCompletion));

      if ((completion.status & 0x1) != (cq.cq_phase ? 1 : 0)) {
        break;
      }

      cq.cq_head = (cq.cq_head + 1) % cq.cq_size;
      if (cq.cq_head == 0) {
        cq.cq_phase = !cq.cq_phase;
      }
      completions++;
    }

    if (completions > 0) {
      *cq.cq_head_db = cq.cq_head;
    }

    return completions;
  }

private:
  /// @brief Allocate DMA memory
  /// @param size Size of memory to allocate
  /// @return Physical address of allocated memory or error
  static fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size) {
    uintptr_t addr = MemoryManager::the().allocate_contiguous((size + 4095) / 4096);
    if (!addr)
      return fk::core::Error::OutOfMemory;
    return addr;
  }
};

/// @brief NVMe Interrupt Handler
class NvmeInterruptHandler {
public:
  /// @brief Handle interrupt from NVMe controller
  /// @param controller Controller to handle interrupt for
  /// @param controller_regs Controller register base
  static void handle_interrupt(InterruptDrivenNvmeController* controller,
                               volatile uint8_t* controller_regs) {
    if (!controller || !controller->is_interrupts_enabled()) {
      return;
    }

    // Clear interrupt
    *reinterpret_cast<volatile uint32_t*>(controller_regs + NVME_INTMC) = 0xFFFFFFFF;

    // Process admin queue completions
    NvmeQueueManager::process_completions(controller->get_admin_cq());

    // Process I/O queue completions
    for (uint32_t i = 0; i < 8; ++i) {
      NvmeCompletionQueue& cq = controller->get_io_cq(i);
      if (cq.cq_memory) {
        NvmeQueueManager::process_completions(cq);
      }
    }
  }
};

/// @brief NVMe DMA Memory Manager
class NvmeDmaMemoryManager {
public:
  /// @brief Allocate DMA memory
  /// @param size Size of memory to allocate
  /// @return Physical address of allocated memory or error
  static fk::core::Result<uintptr_t, fk::core::Error> allocate(size_t size) {
    uintptr_t addr = MemoryManager::the().allocate_contiguous((size + 4095) / 4096);
    if (!addr)
      return fk::core::Error::OutOfMemory;
    return addr;
  }

  /// @brief Free DMA memory
  /// @param phys_addr Physical address of memory to free
  /// @param size Size of memory to free
  static void free(uintptr_t phys_addr, size_t size) {
    MemoryManager::the().free_contiguous(phys_addr, (size + 4095) / 4096);
  }
};

} // namespace fkernel