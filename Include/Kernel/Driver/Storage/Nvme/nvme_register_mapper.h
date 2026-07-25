#pragma once

#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/result.h>

namespace fkernel {

class NvmeRegisterMapper {
public:
  explicit NvmeRegisterMapper(NvmeControllerState& state) : m_state(state) {}

  fk::core::Result<void, fk::core::Error> map_registers() {
    PciDevice& device = m_state.device();
    uint32_t bar0 = PciManager::the().read_config_dword(device.address(), 0x10);
    if ((bar0 & 0x01) != 0) {
      fk::algorithms::kerror("NVMe-REG", "IO space BAR not supported");
      return fk::core::Error::InvalidParameter;
    }
    uintptr_t phys_addr = bar0 & ~0xFu;
    MemoryManager::the().map_page(
        phys_addr, phys_addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    NvmeRegisterAccess new_access(phys_addr);
    m_state.set_register_access(new_access);
    m_state.configuration().set_controller_page_size(new_access.get_page_size());
    uint32_t cmd = PciManager::the().read_config_dword(device.address(), 0x04);
    PciManager::the().write_config_dword(device.address(), 0x04, cmd | 0x06);
    fk::algorithms::klog("NVMe-REG", "Registers mapped");
    return {};
  }

private:
  NvmeControllerState& m_state;
};

} // namespace fkernel