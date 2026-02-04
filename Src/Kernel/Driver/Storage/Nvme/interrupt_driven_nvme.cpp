#include <Kernel/Driver/Storage/Nvme/interrupt_driven_nvme.h>
#include <Kernel/Driver/Storage/Nvme/nvme_command_builder.h>
#include <Kernel/Driver/Storage/Nvme/nvme_completion_processor.h>
#include <Kernel/Driver/Storage/Nvme/nvme_interrupt_handler.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

InterruptDrivenNvmeController* InterruptDrivenNvmeController::create(const PciDevice& device) {
  fk::algorithms::klog("NVMe-INT", "Creating controller for %02x:%02x.%d", device.address().bus(),
                       device.address().device(), device.address().function());

  auto controller_result = fk::make_ref<InterruptDrivenNvmeController>(device);
  if (controller_result.is_error())
    return nullptr;

  auto controller = controller_result.value();
  auto init_result = controller->initialize_interrupt_driven();
  if (init_result.is_error())
    return nullptr;

  fk::algorithms::klog("NVMe-INT", "Controller ready");
  return controller.leak_ptr();
}

InterruptDrivenNvmeController::InterruptDrivenNvmeController(const PciDevice& device)
    : m_state(device) {}

InterruptDrivenNvmeController::~InterruptDrivenNvmeController() {
  NvmeInterruptHandler::unregister_handler(static_cast<uint8_t>(interrupt_line() + 32));
}

fk::core::Result<void, fk::core::Error>
InterruptDrivenNvmeController::initialize_interrupt_driven() {
  auto map_result = map_controller_registers();
  if (map_result.is_error())
    return map_result.error();

  auto config_result = configure_controller();
  if (config_result.is_error())
    return config_result.error();

  auto queue_result = setup_queues();
  if (queue_result.is_error())
    return queue_result.error();

  auto ns_result = identify_namespace();
  if (ns_result.is_error())
    return ns_result.error();

  auto interrupt_result = enable_interrupts();
  if (interrupt_result.is_error())
    return interrupt_result.error();

  NvmeInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenNvmeController>(this));
  m_state.mark_initialized();
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::map_controller_registers() {
  PciDevice& device = m_state.device();
  uint32_t bar0 = PciManager::the().read_config_dword(device.address(), 0x10);
  if ((bar0 & 0x01) != 0) {
    fk::algorithms::kerror("NVMe-INT", "IO space BAR not supported");
    return fk::core::Error::InvalidParameter;
  }

  uintptr_t phys_addr = bar0 & ~0xFu;
  MemoryManager::the().map_page(
      phys_addr, phys_addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);

  NvmeRegisterAccess new_access(phys_addr);
  m_state.set_register_access(new_access);
  uint32_t page_size = new_access.get_page_size();
  m_state.configuration().set_controller_page_size(page_size);

  uint32_t cmd = PciManager::the().read_config_dword(device.address(), 0x04);
  PciManager::the().write_config_dword(device.address(), 0x04, cmd | 0x06);

  fk::algorithms::klog("NVMe-INT", "Registers mapped");
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::configure_controller() {
  auto reset_result = m_state.queue_manager().reset_controller();
  if (reset_result.is_error())
    return reset_result.error();

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_queues() {
  auto admin_result = setup_admin_queue();
  if (admin_result.is_error())
    return admin_result.error();

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_admin_queue() {
  auto result = m_state.queue_manager().setup_admin_queue();
  if (result.is_error())
    return result.error();

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_io_queues() {
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::identify_namespace() {
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::enable_interrupts() {
  PciDevice& device = m_state.device();
  uint32_t interrupt_line = PciManager::the().read_config_byte(device.address(), 0x3C);
  m_state.set_interrupt_line(interrupt_line);
  m_state.register_access().write_intms(0xFFFFFFFF);
  m_state.enable_interrupts();

  fk::algorithms::klog("NVMe-INT", "IRQ %d enabled", interrupt_line);
  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::reset_controller() {
  return m_state.queue_manager().reset_controller();
}

void InterruptDrivenNvmeController::handle_interrupt() {
  if (!m_state.interrupts_enabled())
    return;

  m_state.register_access().write_intmc(0xFFFFFFFF);

  m_state.queue_manager().process_completions();
}

fk::core::Result<uintptr_t, fk::core::Error>
InterruptDrivenNvmeController::allocate_dma_memory(size_t size) {
  uintptr_t addr = MemoryManager::the().allocate_contiguous((size + 4095) / 4096);
  if (!addr)
    return fk::core::Error::OutOfMemory;
  return addr;
}

void InterruptDrivenNvmeController::free_dma_memory(uintptr_t phys_addr, size_t size) {
  MemoryManager::the().free_contiguous(phys_addr, (size + 4095) / 4096);
}

fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
InterruptDrivenNvmeController::submit_read_async(uint64_t start_lba, uint32_t block_count,
                                                 uint8_t* buffer) {
  uint16_t command_id = m_state.command_id_manager().allocate();
  if (command_id == 0xFFFF)
    return fk::core::Error::DeviceBusy;

  auto* operation = new NvmeAsyncOperation(command_id, start_lba, block_count, buffer, false);
  if (!operation) {
    m_state.command_id_manager().release(command_id);
    return fk::core::Error::OutOfMemory;
  }

  m_state.pending_operations().add(operation);

  uint64_t prp1 = MemoryManager::the().translate(reinterpret_cast<uintptr_t>(buffer));

  uint32_t block_size = m_state.configuration().block_size();
  if (block_count * block_size > 4096) {
    fk::algorithms::kerror("NVMe-INT", "Large xfer not supported");
    m_state.command_id_manager().release(command_id);
    return fk::core::Error::NotImplemented;
  }

  NvmeCommand cmd = NvmeCommandBuilder::build_read(start_lba, block_count, prp1, 0,
                                                   m_state.configuration().namespace_id());
  cmd.cdw0 |= (command_id & 0xFFFF) << 16;

  auto submit_result = submit_io_command(cmd);
  if (submit_result.is_error()) {
    m_state.command_id_manager().release(command_id);
    return submit_result.error();
  }

  fk::algorithms::klog("NVMe-INT", "Read: cmd=%d, lba=%ld", command_id, start_lba);
  return operation;
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenNvmeController::read_blocks(uint64_t start_lba, size_t count, uint8_t* buffer) {
  auto async_op_res = submit_read_async(start_lba, static_cast<uint32_t>(count), buffer);
  if (async_op_res.is_error())
    return async_op_res.error();

  auto* async_op = async_op_res.value();
  IoCompletionStatus status = async_op->wait_for_completion(5000);

  if (status == IoCompletionStatus::Success)
    return count;
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
  return m_interrupt_line.value();
}

} // namespace fkernel
