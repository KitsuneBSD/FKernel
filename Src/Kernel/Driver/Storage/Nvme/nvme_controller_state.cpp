#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>

namespace fkernel {

NvmeControllerState::NvmeControllerState(const PciDevice& device)
    : m_pci_device(device), m_register_access(reinterpret_cast<uintptr_t>(nullptr)),
      m_queue_manager(m_register_access) {}

} // namespace fkernel
