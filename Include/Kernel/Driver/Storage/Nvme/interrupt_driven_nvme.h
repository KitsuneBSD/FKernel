#pragma once

#include <Kernel/Driver/async_io.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibC/stdint.h>
#include <LibFK/Container/array.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Memory/ref_counted.h>

// NVMe Specific Headers
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/scheduler.h>

// Forward declare the operation class
class NvmeAsyncOperation;

namespace fkernel {

/// @brief Asynchronous NVMe I/O operation
class NvmeAsyncOperation : public AsyncIoOperation, public fk::memory::RefCounted<NvmeAsyncOperation> {
private:
  uint16_t m_command_id;
  uint64_t m_start_lba;
  uint32_t m_block_count;
  void* m_buffer;
  bool m_is_write;
  IoCompletionStatus m_status = IoCompletionStatus::Busy;
  uint64_t m_completion_time = 0;

public:
  NvmeAsyncOperation(uint16_t command_id, uint64_t start_lba, uint32_t block_count, void* buffer,
                     bool is_write)
      : m_command_id(command_id), m_start_lba(start_lba), m_block_count(block_count),
        m_buffer(buffer), m_is_write(is_write) {}

  uint16_t get_command_id() const { return m_command_id; }
  uint64_t get_start_lba() const { return m_start_lba; }
  uint32_t get_block_count() const { return m_block_count; }
  void* get_buffer() const { return m_buffer; }
  bool is_write() const { return m_is_write; }

  void on_interrupt(uint32_t interrupt_status) override {
    // NVMe uses completion queue entries instead of interrupt status
    if (interrupt_status & 0x1) { // Completion doorbell rung
      m_status = IoCompletionStatus::Success;
      m_completion_time = TickManager::the().get_ticks();
    }
  }

  bool is_completed() const override { return m_status != IoCompletionStatus::Busy; }

  IoCompletionStatus get_status() const override { return m_status; }

  IoCompletionStatus wait_for_completion(uint64_t timeout_ms = 5000) override {
    uint64_t start_time = TickManager::the().get_ticks();
    uint64_t timeout_ticks = (timeout_ms * TickManager::the().get_frequency()) / 1000;

    while (!is_completed()) {
      uint64_t current_time = TickManager::the().get_ticks();
      if ((current_time - start_time) > timeout_ticks) {
        m_status = IoCompletionStatus::Timeout;
        break;
      }

      // Yield CPU to avoid busy-wait
      SchedulerManager::the().yield();
    }

    return m_status;
  }

  void mark_error() { m_status = IoCompletionStatus::Error; }
  void mark_success() { m_status = IoCompletionStatus::Success; }
};

/// @brief NVMe Submission Queue
struct NvmeSubmissionQueue {
  void* sq_memory = nullptr;
  uint16_t sq_size = 0;
  uint16_t sq_head = 0;
  uint16_t sq_tail = 0;
  volatile uint32_t* sq_tail_db = nullptr;
  bool sq_phase = true;
  uint32_t sq_phys_addr = 0;
};

/// @brief NVMe Completion Queue
struct NvmeCompletionQueue {
  void* cq_memory = nullptr;
  uint16_t cq_size = 0;
  uint16_t cq_head = 0;
  uint16_t cq_phase = true;
  volatile uint32_t* cq_head_db = nullptr;
  uint32_t cq_phys_addr = 0;
};

/// @brief NVMe Command Structure (simplified)
struct NvmeCommand {
  uint32_t cdw0;  // Command Dword 0 - Opcode and flags
  uint32_t nsid;  // Namespace ID
  uint64_t mptr;  // Metadata pointer
  uint64_t prp1;  // Physical Region Page 1
  uint64_t prp2;  // Physical Region Page 2
  uint32_t cdw10; // Command Dword 10
  uint32_t cdw11; // Command Dword 11
  uint32_t cdw12; // Command Dword 12
  uint32_t cdw13; // Command Dword 13
  uint32_t cdw14; // Command Dword 14
  uint32_t cdw15; // Command Dword 15
};

/// @brief NVMe Completion Structure
struct NvmeCompletion {
  uint32_t cdw0;       // Command Dword 0
  uint32_t rsvd;       // Reserved
  uint16_t sq_head;    // Submission Queue Head
  uint16_t sq_id;      // Submission Queue ID
  uint16_t command_id; // Command ID
  uint16_t status;     // Status
};

/// @brief Interrupt-driven NVMe Controller
class InterruptDrivenNvmeController : public fk::memory::RefCounted<InterruptDrivenNvmeController> {
private:
  PciDevice m_pci_device;
  volatile uint8_t* m_controller_regs = nullptr;
  uint32_t m_interrupt_line = 0;
  bool m_interrupts_enabled = false;

  // Admin queues (Queue ID 0)
  NvmeSubmissionQueue m_admin_sq;
  NvmeCompletionQueue m_admin_cq;

  // I/O queues (Queue ID 1+)
  fk::containers::array<NvmeSubmissionQueue, 8> m_io_sq;
  fk::containers::array<NvmeCompletionQueue, 8> m_io_cq;

  // Pending operations tracking
  fk::containers::array<NvmeAsyncOperation*, 256> m_pending_operations;

  // Controller information
  uint32_t m_controller_page_size = 4096;
  uint64_t m_total_blocks = 0;
  uint32_t m_block_size = 512;
  uint32_t m_namespace_id = 1;

public:
  static InterruptDrivenNvmeController* create(const PciDevice& device);

  InterruptDrivenNvmeController(const PciDevice& device) : m_pci_device(device) {}
  virtual ~InterruptDrivenNvmeController() override;

  fk::core::Result<void, fk::core::Error> initialize_interrupt_driven();
  fk::core::Result<void, fk::core::Error> enable_interrupts();
  void handle_interrupt();

  // Asynchronous I/O methods
  fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
  submit_read_async(uint64_t start_lba, uint32_t block_count, uint8_t* buffer);

  fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
  submit_write_async(uint64_t start_lba, uint32_t block_count, const uint8_t* buffer);

  // Synchronous I/O methods (using async internally)
  fk::core::Result<size_t, fk::core::Error> read_blocks(uint64_t start_lba, size_t count,
                                                        uint8_t* buffer);

  fk::core::Result<size_t, fk::core::Error> write_blocks(uint64_t start_lba, size_t count,
                                                         const uint8_t* buffer);

  // Device properties
  uint64_t get_total_blocks() const { return m_total_blocks; }
  uint32_t get_block_size() const { return m_block_size; }
  uint32_t get_interrupt_line() const { return m_interrupt_line; }

private:
  fk::core::Result<void, fk::core::Error> reset_controller();
  fk::core::Result<void, fk::core::Error> setup_admin_queue();
  fk::core::Result<void, fk::core::Error> setup_io_queues();
  fk::core::Result<void, fk::core::Error> identify_namespace();

  fk::core::Result<uint16_t, fk::core::Error> submit_admin_command(const NvmeCommand& cmd);
  fk::core::Result<uint16_t, fk::core::Error> submit_io_command(const NvmeCommand& cmd,
                                                                uint32_t queue_id = 1);

  void process_completions();
  void complete_operation(uint16_t command_id, IoCompletionStatus status);

  // Memory management
  fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
  void free_dma_memory(uintptr_t phys_addr, size_t size);

  // Queue operations
  fk::core::Result<void, fk::core::Error> create_submission_queue(NvmeSubmissionQueue& sq,
                                                                  uint16_t size, uint16_t cq_id);
  fk::core::Result<void, fk::core::Error> create_completion_queue(NvmeCompletionQueue& cq,
                                                                  uint16_t size);

  // Command construction
  NvmeCommand build_read_command(uint64_t lba, uint32_t block_count, uint64_t prp1, uint64_t prp2);
  NvmeCommand build_write_command(uint64_t lba, uint32_t block_count, uint64_t prp1, uint64_t prp2);

  uint16_t find_free_command_id();
  void mark_command_id_used(uint16_t command_id);
  void mark_command_id_free(uint16_t command_id);
};

/// @brief NVMe Interrupt Handler
class NvmeInterruptHandler {
private:
  fk::RefPtr<InterruptDrivenNvmeController> m_controller;
  uint32_t m_irq_vector;

public:
  NvmeInterruptHandler(fk::RefPtr<InterruptDrivenNvmeController> controller, uint32_t irq)
      : m_controller(controller), m_irq_vector(irq) {}

  void handle_interrupt();
  uint32_t get_irq_vector() const { return m_irq_vector; }

  static void register_handler(fk::RefPtr<InterruptDrivenNvmeController> controller);
};

} // namespace fkernel