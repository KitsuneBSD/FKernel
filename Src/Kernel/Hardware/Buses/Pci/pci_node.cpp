#include <Kernel/Hardware/Buses/Pci/pci_node.h>
#include <Kernel/Hardware/Buses/Pci/pci.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Algorithms/Generic/math.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

PCIDeviceNode::PCIDeviceNode() {
    set_name("pci");
}

fk::core::Result<size_t, fk::core::Error> PCIDeviceNode::read(uint64_t, size_t size, uint8_t* buffer) {
    if (m_events.is_empty()) {
        return 0;
    }

    size_t events_to_read = size / sizeof(PCIEvent);
    if (events_to_read == 0) return 0;

    size_t actual_events = fk::algorithms::min(events_to_read, m_events.size());
    size_t total_bytes = actual_events * sizeof(PCIEvent);

    for (size_t i = 0; i < actual_events; ++i) {
        PCIEvent event = m_events.dequeue();
        fk::memory::copy(buffer + (i * sizeof(PCIEvent)), &event, sizeof(PCIEvent));
    }

    return total_bytes;
}

size_t PCIDeviceNode::size() const {
    return m_events.size() * sizeof(PCIEvent);
}

void PCIDeviceNode::push_event(const PCIEvent& event) {
    m_events.enqueue(event);
}

fk::core::Result<int, fk::core::Error> PCIDeviceNode::ioctl(uint64_t request, uint64_t arg) {
    if (request != PIOC_READ_CONFIG && request != PIOC_WRITE_CONFIG)
        return fk::core::Error::InappropriateIoctlForDevice;

    PiocConfigOp op{};
    auto res = fkernel::memory::copy_from_user(&op, reinterpret_cast<const void*>(arg), sizeof(PiocConfigOp));
    if (res.is_error()) return res.error();

    if (op.width != 1 && op.width != 2 && op.width != 4)
        return fk::core::Error::InvalidParameter;
    if (op.offset > 255)
        return fk::core::Error::InvalidParameter;

    PciAddress addr(op.bus, op.dev, op.fn);

    if (request == PIOC_READ_CONFIG) {
        if (op.width == 1)
            op.value = PciManager::the().read_config_byte(addr, static_cast<uint8_t>(op.offset));
        else if (op.width == 2)
            op.value = PciManager::the().read_config_word(addr, static_cast<uint8_t>(op.offset));
        else
            op.value = PciManager::the().read_config_dword(addr, static_cast<uint8_t>(op.offset));

        auto wres = fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &op, sizeof(PiocConfigOp));
        if (wres.is_error()) return wres.error();
    } else {
        if (op.width == 1)
            PciManager::the().write_config_byte(addr, static_cast<uint8_t>(op.offset), static_cast<uint8_t>(op.value));
        else if (op.width == 2)
            PciManager::the().write_config_word(addr, static_cast<uint8_t>(op.offset), static_cast<uint16_t>(op.value));
        else
            PciManager::the().write_config_dword(addr, static_cast<uint8_t>(op.offset), op.value);
    }

    return 0;
}

} // namespace fkernel
