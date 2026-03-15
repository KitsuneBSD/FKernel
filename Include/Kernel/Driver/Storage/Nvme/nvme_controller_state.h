#pragma once

#include <Kernel/Driver/Storage/Nvme/NvmeQueueManager.h>
#include <Kernel/Driver/Storage/Nvme/NvmeRegisterAccess.h>
#include <Kernel/Driver/Storage/Nvme/nvme_command_id_manager.h>
#include <Kernel/Driver/Storage/Nvme/nvme_device_configuration.h>
#include <Kernel/Driver/Storage/Nvme/nvme_pending_operations.h>
#include <Kernel/Hardware/Pci/pci_device.h>

namespace fkernel {

class NvmeControllerState {
public:
  NvmeControllerState(const PciDevice& device);

  NvmeQueueManager& queue_manager() { return m_queue_manager; }
  const NvmeQueueManager& queue_manager() const { return m_queue_manager; }
  NvmeDeviceConfiguration& configuration() { return m_configuration; }
  const NvmeDeviceConfiguration& configuration() const { return m_configuration; }
  NvmeCommandIdManager& command_id_manager() { return m_command_id_manager; }
  const NvmeCommandIdManager& command_id_manager() const { return m_command_id_manager; }
  NvmePendingOperations& pending_operations() { return m_pending_operations; }
  const NvmePendingOperations& pending_operations() const { return m_pending_operations; }

  bool interrupts_enabled() const { return m_interrupts_enabled; }
  void enable_interrupts() { m_interrupts_enabled = true; }
  void disable_interrupts() { m_interrupts_enabled = false; }

  uint32_t interrupt_line() const { return m_interrupt_line; }
  void set_interrupt_line(uint32_t line) { m_interrupt_line = line; }

  PciDevice& device() { return m_pci_device; }

  void set_register_access(const NvmeRegisterAccess& access) { m_register_access = access; }
  NvmeRegisterAccess& register_access() { return m_register_access; }

  bool is_initialized() const { return m_initialized; }
  void mark_initialized() { m_initialized = true; }

private:
  PciDevice m_pci_device;
  NvmeRegisterAccess m_register_access;
  NvmeQueueManager m_queue_manager;
  NvmeDeviceConfiguration m_configuration;
  NvmeCommandIdManager m_command_id_manager;
  NvmePendingOperations m_pending_operations;
  bool m_interrupts_enabled = false;
  uint32_t m_interrupt_line = 0;
  bool m_initialized = false;
};

} // namespace fkernel
