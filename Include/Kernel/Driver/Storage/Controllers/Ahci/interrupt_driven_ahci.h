#pragma once

#include <Kernel/Driver/Storage/Controllers/Ahci/ahci_controller.h>
#include <Kernel/Driver/Async/async_io.h>
#include <LibFK/Types/types.h>
#include <LibFK/Container/Sequence/array.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Memory/Pointers/ref_counted.h>

namespace fkernel {

/// @brief Asynchronous AHCI I/O operation
class AhciAsyncOperation : public AsyncIoOperation, public fk::memory::RefCounted<AhciAsyncOperation> {
private:
  uint32_t m_port_index;
  uint32_t m_slot;
  uint64_t m_start_sector;
  uint32_t m_sector_count;
  void* m_buffer;
  bool m_is_write;
  IoCompletionStatus m_status = IoCompletionStatus::Busy;
  uint64_t m_completion_time = 0;

public:
  AhciAsyncOperation(uint32_t port_index, uint32_t slot, uint64_t start_sector,
                     uint32_t sector_count, void* buffer, bool is_write)
      : m_port_index(port_index), m_slot(slot), m_start_sector(start_sector),
        m_sector_count(sector_count), m_buffer(buffer), m_is_write(is_write) {}

  uint32_t get_slot() const { return m_slot; }
  uint32_t get_port_index() const { return m_port_index; }
  uint64_t get_start_sector() const { return m_start_sector; }
  uint32_t get_sector_count() const { return m_sector_count; }
  void* get_buffer() const { return m_buffer; }
  bool is_write() const { return m_is_write; }

  void on_interrupt(uint32_t interrupt_status) override {
    // Mark as completed if this operation's port/slot completed
    if (interrupt_status & (1 << m_port_index)) {
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
};

/// @brief Interrupt-driven AHCI Controller
class InterruptDrivenAhciController : public AHCIController {
private:
  using IoQueue = AsyncIoQueue<fk::RefPtr<AhciAsyncOperation>, 32>;

  fk::containers::array<IoQueue, 32> m_port_queues;
  fk::containers::array<DmaBuffer, 32> m_port_buffers;
  uint32_t m_interrupt_line = 0;
  bool m_interrupts_enabled = false;

public:
  static fk::RefPtr<InterruptDrivenAhciController> create(const PciDevice& device);

  InterruptDrivenAhciController(const PciDevice& device) : AHCIController(device) {}
  virtual ~InterruptDrivenAhciController() override;

  fk::core::Result<void, fk::core::Error> initialize_interrupt_driven();
  fk::core::Result<void, fk::core::Error> enable_interrupts();
  void handle_interrupt(uint32_t interrupt_status);

  // Asynchronous I/O methods
  fk::core::Result<fk::RefPtr<AhciAsyncOperation>, fk::core::Error>
  submit_read_async(uint32_t port_index, uint64_t start_sector, uint32_t count, uint8_t* buffer);

  fk::core::Result<fk::RefPtr<AhciAsyncOperation>, fk::core::Error>
  submit_write_async(uint32_t port_index, uint64_t start_sector, uint32_t count,
                     const uint8_t* buffer);

  // Synchronous I/O methods (using async internally)
  fk::core::Result<size_t, fk::core::Error> read_sectors_async(uint64_t start_sector, size_t count,
                                                               uint8_t* buffer);

  fk::core::Result<size_t, fk::core::Error> write_sectors_async(uint64_t start_sector, size_t count,
                                                                const uint8_t* buffer) ;

  uint32_t get_interrupt_line() const { return m_interrupt_line; }

private:
  fk::core::Result<uint32_t, fk::core::Error> find_free_slot(uint32_t port_index);
  void setup_command_header(uint32_t port_index, uint32_t slot, uint32_t sector_count,
                            bool is_write);
  void setup_fis(uint32_t port_index, uint32_t slot, uint64_t lba, uint32_t count, bool is_write);
  void start_command(uint32_t port_index, uint32_t slot);

  // Interrupt completion handling
  void complete_operation(uint32_t port_index, uint32_t slot, IoCompletionStatus status);
  void process_completions(uint32_t port_index);

  // DMA buffer management
  fk::core::Result<void, fk::core::Error> setup_dma_buffers();
  DmaBuffer& get_dma_buffer(uint32_t port_index);
};

/// @brief AHCI Interrupt Handler
class AhciInterruptHandler {
private:
  fk::RefPtr<InterruptDrivenAhciController> m_controller;
  uint32_t m_irq_vector;

public:
  AhciInterruptHandler(fk::RefPtr<InterruptDrivenAhciController> controller, uint32_t irq)
      : m_controller(controller), m_irq_vector(irq) {}

  void handle_interrupt();
  uint32_t get_irq_vector() const { return m_irq_vector; }

  static void register_handler(fk::RefPtr<InterruptDrivenAhciController> controller);
};

} // namespace fkernel