#pragma once

#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Core/result.h>

namespace fkernel {

class NvmeInterruptConfigurator {
public:
  explicit NvmeInterruptConfigurator(NvmeControllerState& state) : m_state(state) {}

  fk::core::Result<void, fk::core::Error> configure_interrupts() {
    PciDevice& device = m_state.device();
    uint32_t interrupt_line = PciManager::the().read_config_byte(device.address(), 0x3C);
    m_state.set_interrupt_line(interrupt_line);
    m_state.register_access().write_intms(0xFFFFFFFF);
    m_state.enable_interrupts();
    fk::algorithms::klog("NVMe-INT", "IRQ %d enabled", interrupt_line);
    return {};
  }

private:
  NvmeControllerState& m_state;
};

} // namespace fkernel