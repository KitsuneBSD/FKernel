#include <Kernel/Block/PartitionDeviceList.h>
#include <Kernel/Block/PartitionDevice.h>

namespace fkernel::block {
class PartitionBlockDevice;
}

void PartitionDeviceList::add(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device) {
    if (m_devices.size() < m_devices.capacity()) {
        m_devices.push_back(fk::types::move(device));
    }
}
