#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_controller_state.h>

namespace fkernel {

NvmeControllerState::NvmeControllerState(const PciDevice& device)
    : m_pci_device(device), m_register_access(0),
      m_queue_manager(m_register_access) {}

} // namespace fkernel
