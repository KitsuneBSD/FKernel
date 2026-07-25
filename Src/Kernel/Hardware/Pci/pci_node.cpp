#include <Kernel/Hardware/Pci/pci_node.h>
#include <LibFK/Algorithms/math.h>
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

} // namespace fkernel
