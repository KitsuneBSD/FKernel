#pragma once

#include <Kernel/Block/PartitionDevice.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>

namespace fkernel::block {
class PartitionBlockDevice;
}

class PartitionDeviceList {
public:
    using PartitionVector = fk::containers::static_vector<fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>, 16>;

    void add(fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice> device);

    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>* begin() { return m_devices.begin(); }
    fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>* end() { return m_devices.end(); }
    const fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>* begin() const { return m_devices.begin(); }
    const fk::memory::RetainPtr<fkernel::block::PartitionBlockDevice>* end() const { return m_devices.end(); }

    bool is_empty() const { return m_devices.size() == 0; }
    size_t count() const { return m_devices.size(); }

private:
    PartitionVector m_devices;
};
