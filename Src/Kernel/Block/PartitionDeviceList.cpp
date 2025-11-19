#include <Kernel/Block/PartitionDeviceList.h>

void PartitionDeviceList::add(fk::memory::RetainPtr<PartitionBlockDevice> device) {
    if (m_devices.size() < m_devices.capacity()) {
        m_devices.push_back(fk::types::move(device));
    }
}
