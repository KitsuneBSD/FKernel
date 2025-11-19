#pragma once

#include <Kernel/Block/PartitionDevice.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>

class PartitionDeviceList {
public:
    using PartitionVector = fk::containers::static_vector<fk::memory::RetainPtr<PartitionBlockDevice>, 16>;

    void add(fk::memory::RetainPtr<PartitionBlockDevice> device);

    fk::memory::RetainPtr<PartitionBlockDevice>* begin() { return m_devices.begin(); }
    fk::memory::RetainPtr<PartitionBlockDevice>* end() { return m_devices.end(); }
    const fk::memory::RetainPtr<PartitionBlockDevice>* begin() const { return m_devices.begin(); }
    const fk::memory::RetainPtr<PartitionBlockDevice>* end() const { return m_devices.end(); }

    bool is_empty() const { return m_devices.size() == 0; }
    size_t count() const { return m_devices.size(); }

private:
    PartitionVector m_devices;
};
