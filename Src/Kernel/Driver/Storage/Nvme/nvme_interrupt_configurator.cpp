#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>
#include <Kernel/Driver/Storage/Nvme/nvme_interrupt_configurator.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Hardware/Pci/pci_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

fk::core::Result<void, fk::core::Error> NvmeInterruptConfigurator::configure_interrupts() {
  PciDevice& device = m_state.device();
  uint32_t interrupt_line = PciManager::the().read_config_byte(device.address(), 0x3C);
  m_state.set_interrupt_line(interrupt_line);
  m_state.register_access().write_intms(0xFFFFFFFF);
  m_state.enable_interrupts();
  fk::algorithms::klog("NVMe-INT", "IRQ %d enabled", interrupt_line);
  return {};
}

} // namespace fkernel