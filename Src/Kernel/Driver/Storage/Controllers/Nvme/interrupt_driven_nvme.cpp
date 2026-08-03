#include <Kernel/Driver/Storage/Controllers/Nvme/interrupt_driven_nvme.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command_builder.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_completion_processor.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_interrupt_configurator.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_interrupt_handler.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_setup.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_register_mapper.h>
#include <Kernel/Hardware/Buses/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <LibFK/Algorithms/Logging/log.h>

namespace fkernel {

InterruptDrivenNvmeController* InterruptDrivenNvmeController::create(const PciDevice& device) {
  fk::algorithms::klog("NVME_INT", "Creating controller for %02x:%02x.%d", device.address().bus(),
                       device.address().device(), device.address().function());
  auto controller_result = fk::make_ref<InterruptDrivenNvmeController>(device);
  if (controller_result.is_error())
    return nullptr;
  auto controller = controller_result.value();
  auto init_result = controller->initialize_interrupt_driven();
  if (init_result.is_error())
    return nullptr;
  fk::algorithms::klog("NVME_INT", "Controller ready");
  return controller.leak_ptr();
}

InterruptDrivenNvmeController::InterruptDrivenNvmeController(const PciDevice& device)
    : m_state(device) {}

InterruptDrivenNvmeController::~InterruptDrivenNvmeController() {
  NvmeInterruptHandler::unregister_handler(static_cast<uint8_t>(m_interrupt_line.to_uint32() + 32));
}

fk::core::Result<void, fk::core::Error>
InterruptDrivenNvmeController::initialize_interrupt_driven() {
  TRY(map_controller_registers());
  TRY(configure_controller());
  TRY(setup_queues());
  TRY(identify_namespace());
  TRY(enable_interrupts());
  NvmeInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenNvmeController>(this));
  m_state.mark_initialized();
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::map_controller_registers() {
  NvmeRegisterMapper mapper(m_state);
  return mapper.map_registers();
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::configure_controller() {
  return m_state.queue_manager().reset_controller();
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_queues() {
  NvmeQueueSetup queue_setup(m_state);
  return queue_setup.setup_queues();
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::identify_namespace() {
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::enable_interrupts() {
  NvmeInterruptConfigurator configurator(m_state);
  return configurator.configure_interrupts();
}

void InterruptDrivenNvmeController::handle_interrupt() {
  if (!m_state.interrupts_enabled())
    return;
  m_state.register_access().write_intmc(0xFFFFFFFF);
  m_state.queue_manager().process_completions();
}

fk::core::Result<uintptr_t, fk::core::Error>
InterruptDrivenNvmeController::allocate_dma_memory(size_t size) {
  uintptr_t addr = MemoryManager::the().allocate_contiguous(size_to_order(size));
  if (!addr)
    return fk::core::Error::OutOfMemory;
  return addr;
}

void InterruptDrivenNvmeController::free_dma_memory(uintptr_t phys_addr, size_t size) {
  MemoryManager::the().free_contiguous(phys_addr, size_to_order(size));
}

fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
InterruptDrivenNvmeController::submit_read_async(uint64_t start_lba, uint32_t block_count,
                                                 uint8_t* buffer) {
  uint16_t command_id = m_state.command_id_manager().allocate();
  if (command_id == 0xFFFF)
    return fk::core::Error::DeviceBusy;

  auto operation_result = create_read_operation(command_id, start_lba, block_count, buffer);
  if (operation_result.is_error()) {
    m_state.command_id_manager().release(command_id);
    return operation_result.error();
  }

  auto* operation = operation_result.value();
  m_state.pending_operations().add(operation);

  auto submit_result = submit_read_command(operation);
  if (submit_result.is_error()) {
    m_state.command_id_manager().release(command_id);
    return submit_result.error();
  }

  fk::algorithms::klog("NVME_INT", "Read: cmd=%d, lba=%ld", command_id, start_lba);
  return operation;
}

fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
InterruptDrivenNvmeController::create_read_operation(uint16_t command_id, uint64_t start_lba,
                                                     uint32_t block_count, uint8_t* buffer) {
  auto* operation = new NvmeAsyncOperation(command_id, start_lba, block_count, buffer, false);
  if (!operation)
    return fk::core::Error::OutOfMemory;
  return operation;
}

fk::core::Result<void, fk::core::Error>
InterruptDrivenNvmeController::submit_read_command(NvmeAsyncOperation* operation) {
  uint64_t prp1 = MemoryManager::the().translate(reinterpret_cast<uintptr_t>(operation->buffer()));
  uint32_t transfer_size = operation->block_count() * m_state.configuration().block_size();
  if (transfer_size > 4096) {
    fk::algorithms::kerror("NVME_INT", "Large xfer not supported");
    return fk::core::Error::NotImplemented;
  }

  NvmeCommand cmd = NvmeCommandBuilder::build_read(operation->start_lba(), operation->block_count(),
                                                   prp1, 0, m_state.configuration().namespace_id());
  cmd.cdw0 |= (operation->command_id() & 0xFFFF) << 16;

  auto submit_result = submit_io_command(cmd);
  if (submit_result.is_error())
    return submit_result.error();

  return {};
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenNvmeController::read_blocks(uint64_t start_lba, size_t count, uint8_t* buffer) {
  auto async_op_res = submit_read_async(start_lba, static_cast<uint32_t>(count), buffer);
  if (async_op_res.is_error())
    return async_op_res.error();
  auto* async_op = async_op_res.value();
  IoCompletionStatus status = async_op->wait_for_completion(5000);
  return translate_completion_status(status, count);
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenNvmeController::translate_completion_status(IoCompletionStatus status,
                                                           size_t block_count) {
  if (status == IoCompletionStatus::Success)
    return block_count;
  if (status == IoCompletionStatus::Timeout)
    return fk::core::Error::Timeout;
  return fk::core::Error::DeviceError;
}

fk::core::Result<uint16_t, fk::core::Error>
InterruptDrivenNvmeController::submit_io_command(const NvmeCommand& cmd) {
  auto result = m_state.queue_manager().submit_command(&cmd, 1);
  if (result.is_error())
    return result.error();
  return static_cast<uint16_t>(0);
}

bool InterruptDrivenNvmeController::has_device_info() const {
  return m_state.configuration().is_valid();
}

uint64_t InterruptDrivenNvmeController::total_blocks() const {
  return m_state.configuration().total_blocks();
}

uint32_t InterruptDrivenNvmeController::block_size() const {
  return m_state.configuration().block_size();
}

uint32_t InterruptDrivenNvmeController::interrupt_line() const {
  return m_interrupt_line.to_uint32();
}

} // namespace fkernel
