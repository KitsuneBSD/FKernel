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
                                        uint64_t prp2 = 0);

  /// @brief Build a write command
  /// @param lba Starting logical block address
  /// @param block_count Number of blocks to write
  /// @param prp1 First PRP entry
  /// @param prp2 Second PRP entry (optional)
  /// @return NvmeCommand with write operation
  static NvmeCommand build_write_command(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                         uint64_t prp2 = 0);

  /// @brief Build a flush command
  /// @return NvmeCommand with flush operation
  static NvmeCommand build_flush_command();
};

/// @brief NVMe Queue Manager
class NvmeQueueManager {
public:
  /// @brief Create submission queue
  /// @param sq Submission queue structure to initialize
  /// @param size Number of entries in the queue
  /// @param cq_id Associated completion queue ID
  /// @param controller_regs Controller register base
  /// @return Result indicating success or failure
  static fk::core::Result<void, fk::core::Error>
  create_submission_queue(NvmeSubmissionQueue& sq, uint16_t size, uint16_t cq_id,
                          volatile uint8_t* controller_regs);

  /// @brief Create completion queue
  /// @param cq Completion queue structure to initialize
  /// @param size Number of entries in the queue
  /// @param controller_regs Controller register base
  /// @return Result indicating success or failure
  static fk::core::Result<void, fk::core::Error>
  create_completion_queue(NvmeCompletionQueue& cq, uint16_t size,
                          volatile uint8_t* controller_regs);

  /// @brief Process completions from a completion queue
  /// @param cq Completion queue to process
  /// @return Number of completions processed
  static uint32_t process_completions(NvmeCompletionQueue& cq);

private:
  /// @brief Allocate DMA memory
  /// @param size Size of memory to allocate
  /// @return Physical address of allocated memory or error
  static fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
};

/// @brief NVMe Interrupt Handler
class NvmeInterruptHandler {
public:
  /// @brief Handle interrupt from NVMe controller
  /// @param controller Controller to handle interrupt for
  /// @param controller_regs Controller register base
  static void handle_interrupt(InterruptDrivenNvmeController* controller,
                               volatile uint8_t* controller_regs);
};

/// @brief NVMe DMA Memory Manager
class NvmeDmaMemoryManager {
public:
  /// @brief Allocate DMA memory
  /// @param size Size of memory to allocate
  /// @return Physical address of allocated memory or error
  static fk::core::Result<uintptr_t, fk::core::Error> allocate(size_t size);

  /// @brief Free DMA memory
  /// @param phys_addr Physical address of memory to free
  /// @param size Size of memory to free
  static void free(uintptr_t phys_addr, size_t size);
};

} // namespace fkernel