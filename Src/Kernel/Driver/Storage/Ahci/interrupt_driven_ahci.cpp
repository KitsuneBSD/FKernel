#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Driver/Storage/Ahci/interrupt_driven_ahci.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

static InterruptDrivenAhciController* s_ahci_controllers[MAX_x86_64_IDT_SIZE] = { nullptr };

static void ahci_interrupt_dispatcher(uint8_t vector, InterruptFrame* frame) {
  if (s_ahci_controllers[vector]) {
    s_ahci_controllers[vector]->handle_interrupt(*reinterpret_cast<uint32_t*>(&frame->rdi)); 
  }
}

fk::RefPtr<InterruptDrivenAhciController>
InterruptDrivenAhciController::create(const PciDevice& device) {
  fk::algorithms::klog(
      "AHCI-INT", "Creating interrupt-driven AHCI controller for device %02x:%02x.%d",
      device.address().bus(), device.address().device(), device.address().function());

  auto controller_result = fk::make_ref<InterruptDrivenAhciController>(device);
  if (controller_result.is_error()) {
    fk::algorithms::kerror("AHCI-INT", "Failed to create controller instance");
    return nullptr;
  }

  auto controller = controller_result.value();
  auto init_result = controller->initialize_interrupt_driven();
  if (init_result.is_error()) {
    fk::algorithms::kerror("AHCI-INT", "Failed to initialize controller: %d",
                           (int)init_result.error());
    return nullptr;
  }

  fk::algorithms::klog("AHCI-INT", "Interrupt-driven controller created successfully");
  return controller;
}

InterruptDrivenAhciController::~InterruptDrivenAhciController() {
    uint8_t vector = m_interrupt_line + 32;
    if (s_ahci_controllers[vector] == this) {
        s_ahci_controllers[vector] = nullptr;
    }
}

fk::core::Result<void, fk::core::Error>
InterruptDrivenAhciController::initialize_interrupt_driven() {
  auto base_result = initialize_controller();
  if (base_result.is_error()) {
    return base_result.error();
  }

  auto buffer_result = setup_dma_buffers();
  if (buffer_result.is_error()) {
    return buffer_result.error();
  }

  auto interrupt_result = enable_interrupts();
  if (interrupt_result.is_error()) {
    return interrupt_result.error();
  }

  AhciInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenAhciController>(this));

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenAhciController::enable_interrupts() {
  m_interrupt_line = PciManager::the().read_config_byte(m_pci_device.address(), 0x3C);

  uint16_t command = PciManager::the().read_config_word(m_pci_device.address(), 0x04);
  command |= 0x0100;
  PciManager::the().write_config_word(m_pci_device.address(), 0x04, command);

  uint32_t ports_implemented =
      *reinterpret_cast<volatile uint32_t*>(m_hba_base + 0x0C);

  for (uint32_t i = 0; i < 32; ++i) {
    if ((ports_implemented & (1u << i)) == 0)
      continue;

    auto& port = m_ports[i];
    if (!port.is_implemented)
      continue;

    port.regs->ie = 0x7FFFF;
    fk::algorithms::klog("AHCI-INT", "Enabled interrupts for port %d", i);
  }

  uint32_t ghc = *reinterpret_cast<volatile uint32_t*>(m_hba_base + 0x04);
  ghc |= (1u << 1);
  *reinterpret_cast<volatile uint32_t*>(m_hba_base + 0x04) = ghc;

  m_interrupts_enabled = true;
  fk::algorithms::klog("AHCI-INT", "Interrupts enabled on IRQ %d", m_interrupt_line);

  return {};
}

void InterruptDrivenAhciController::handle_interrupt(uint32_t interrupt_status) {
  *reinterpret_cast<volatile uint32_t*>(m_hba_base + 0x08) = interrupt_status;

  for (uint32_t port_idx = 0; port_idx < 32; ++port_idx) {
    if ((interrupt_status & (1u << port_idx)) == 0)
      continue;

    auto& port = m_ports[port_idx];
    if (!port.is_implemented)
      continue;

    uint32_t port_is = port.regs->is;
    port.regs->is = port_is;

    if (port_is & (1u << 0)) {
      process_completions(port_idx);
    }

    if (port_is & (1u << 30)) {
      process_completions(port_idx); 
    }
  }
}

fk::core::Result<fk::RefPtr<AhciAsyncOperation>, fk::core::Error>
InterruptDrivenAhciController::submit_read_async(uint32_t port_index, uint64_t start_sector,
                                                 uint32_t count, uint8_t* buffer) {
  auto slot_result = find_free_slot(port_index);
  if (slot_result.is_error()) {
    return slot_result.error();
  }

  uint32_t slot = slot_result.value();

  auto operation =
      fk::make_ref<AhciAsyncOperation>(port_index, slot, start_sector, count, buffer, false);
  if (operation.is_error()) {
    return fk::core::Error::OutOfMemory;
  }

  if (!m_port_queues[port_index].enqueue(operation.value())) {
    return fk::core::Error::DeviceBusy;
  }

  setup_command_header(port_index, slot, count, false);
  setup_fis(port_index, slot, start_sector, count, false);
  start_command(port_index, slot);

  return operation;
}

fk::core::Result<fk::RefPtr<AhciAsyncOperation>, fk::core::Error>
InterruptDrivenAhciController::submit_write_async(uint32_t port_index, uint64_t start_sector,
                                                  uint32_t count, const uint8_t* buffer) {
  auto slot_result = find_free_slot(port_index);
  if (slot_result.is_error()) {
    return slot_result.error();
  }

  uint32_t slot = slot_result.value();

  auto operation = fk::make_ref<AhciAsyncOperation>(port_index, slot, start_sector, count,
                                                    const_cast<uint8_t*>(buffer), true);
  if (operation.is_error()) {
    return fk::core::Error::OutOfMemory;
  }

  if (!m_port_queues[port_index].enqueue(operation.value())) {
    return fk::core::Error::DeviceBusy;
  }

  setup_command_header(port_index, slot, count, true);
  setup_fis(port_index, slot, start_sector, count, true);
  start_command(port_index, slot);

  return operation;
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenAhciController::read_sectors_async(uint64_t start_sector, size_t count,
                                                  uint8_t* buffer) {
  if (m_ports.is_empty())
    return fk::core::Error::NotFound;

  auto async_op_res = submit_read_async(0, start_sector, (uint32_t)count, buffer);
  if (async_op_res.is_error()) {
    return async_op_res.error();
  }

  auto async_op = async_op_res.value();

  IoCompletionStatus status = async_op->wait_for_completion(5000);

  if (status == IoCompletionStatus::Success) {
    return count;
  } else if (status == IoCompletionStatus::Timeout) {
    return fk::core::Error::Timeout;
  } else {
    return fk::core::Error::DeviceError;
  }
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenAhciController::write_sectors_async(uint64_t start_sector, size_t count,
                                                   const uint8_t* buffer) {
  if (m_ports.is_empty())
    return fk::core::Error::NotFound;

  auto async_op_res = submit_write_async(0, start_sector, (uint32_t)count, buffer);
  if (async_op_res.is_error()) {
    return async_op_res.error();
  }

  auto async_op = async_op_res.value();

  IoCompletionStatus status = async_op->wait_for_completion(5000);

  if (status == IoCompletionStatus::Success) {
    return count;
  } else if (status == IoCompletionStatus::Timeout) {
    return fk::core::Error::Timeout;
  } else {
    return fk::core::Error::DeviceError;
  }
}

fk::core::Result<uint32_t, fk::core::Error>
InterruptDrivenAhciController::find_free_slot(uint32_t port_index) {
  if (port_index >= m_ports.size() || !m_ports[port_index].is_implemented) {
    return fk::core::Error::InvalidParameter;
  }

  auto& port = m_ports[port_index];
  uint32_t slots_active = port.regs->ci;
  uint32_t slots_implemented =
      *reinterpret_cast<volatile uint32_t*>(m_hba_base + 0x0C);

  for (uint32_t slot = 0; slot < 32; ++slot) {
    if ((slots_implemented & (1u << slot)) == 0)
      continue; 
    if ((slots_active & (1u << slot)) == 0)
      return slot; 
  }

  return fk::core::Error::DeviceBusy;
}

void InterruptDrivenAhciController::process_completions(uint32_t port_index) {
  auto& queue = m_port_queues[port_index];
  auto& port = m_ports[port_index];

  uint32_t slots_active = port.regs->ci;

  for (uint32_t i = 0; i < queue.size(); ++i) {
    auto* operation_ptr = queue.find_pending(i);
    if (!operation_ptr)
      continue;

    auto& operation = *operation_ptr;
    uint32_t slot = operation->get_slot();

    if ((slots_active & (1u << slot)) == 0) {
      uint32_t tfd = port.regs->tfd;
      if (tfd & (1u << 0)) { 
        operation->mark_error();
      }

      operation->on_interrupt(1u << port_index);
      queue.dequeue();
    }
  }
}

void InterruptDrivenAhciController::complete_operation(uint32_t port_index, uint32_t slot,
                                                       IoCompletionStatus status) {
  auto& queue = m_port_queues[port_index];
  auto* operation_ptr = queue.find_pending(slot);

  if (operation_ptr) {
    auto& operation = *operation_ptr;

    if (status == IoCompletionStatus::Success) {
      auto& port = m_ports[port_index];
      uint32_t tfd = port.regs->tfd;
      if (tfd & (1u << 0)) { 
        operation->mark_error();
      }
    }

    operation->on_interrupt(1u << port_index);
  }
}

void AhciInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenAhciController> controller) {
  uint32_t irq = controller->get_interrupt_line();
  if (irq == 0 || irq >= 32) {
    fk::algorithms::kerror("AHCI-INT", "Invalid IRQ line: %d", irq);
    return;
  }

  uint8_t vector = irq + 32;
  s_ahci_controllers[vector] = controller.ptr();

  InterruptController::the().register_interrupt(ahci_interrupt_dispatcher, vector);

  fk::algorithms::klog("AHCI-INT", "Registered interrupt handler for IRQ %d (vector %d)", irq, vector);
}

// Stubs para o linker
fk::core::Result<void, fk::core::Error> InterruptDrivenAhciController::setup_dma_buffers() { return {}; }
void InterruptDrivenAhciController::setup_command_header(uint32_t, uint32_t, uint32_t, bool) {}
void InterruptDrivenAhciController::setup_fis(uint32_t, uint32_t, uint64_t, uint32_t, bool) {}
void InterruptDrivenAhciController::start_command(uint32_t, uint32_t) {}
DmaBuffer& InterruptDrivenAhciController::get_dma_buffer(uint32_t) { static DmaBuffer dummy; return dummy; }

} // namespace fkernel
