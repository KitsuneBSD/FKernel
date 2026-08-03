#pragma once

#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_async_operation.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_controller_state.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_interrupt_line.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/Pointers/ref_counted.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

namespace fkernel {

class InterruptDrivenNvmeController : public fk::memory::RefCounted<InterruptDrivenNvmeController> {
public:
  static InterruptDrivenNvmeController* create(const PciDevice& device);

  explicit InterruptDrivenNvmeController(const PciDevice& device);
  virtual ~InterruptDrivenNvmeController() override;

  fk::core::Result<void, fk::core::Error> initialize_interrupt_driven();
  void handle_interrupt();

  fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
  submit_read_async(uint64_t start_lba, uint32_t block_count, uint8_t* buffer);

  fk::core::Result<size_t, fk::core::Error> read_blocks(uint64_t start_lba, size_t count,
                                                        uint8_t* buffer);

  bool has_device_info() const;
  uint64_t total_blocks() const;
  uint32_t block_size() const;
  uint32_t interrupt_line() const;

private:
  fk::core::Result<void, fk::core::Error> map_controller_registers();
  fk::core::Result<void, fk::core::Error> configure_controller();
  fk::core::Result<void, fk::core::Error> setup_queues();
  fk::core::Result<void, fk::core::Error> identify_namespace();
  fk::core::Result<void, fk::core::Error> enable_interrupts();
  fk::core::Result<void, fk::core::Error> setup_admin_queue();

  fk::core::Result<uint16_t, fk::core::Error> submit_io_command(const NvmeCommand& cmd);
  fk::core::Result<size_t, fk::core::Error> translate_completion_status(IoCompletionStatus status,
                                                                        size_t block_count);

  fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
  void free_dma_memory(uintptr_t phys_addr, size_t size);

  fk::core::Result<NvmeAsyncOperation*, fk::core::Error> create_read_operation(uint16_t command_id,
                                                                               uint64_t start_lba,
                                                                               uint32_t block_count,
                                                                               uint8_t* buffer);
  fk::core::Result<void, fk::core::Error> submit_read_command(NvmeAsyncOperation* operation);

  NvmeControllerState m_state;
  NvmeInterruptLine m_interrupt_line{0};
};

} // namespace fkernel
