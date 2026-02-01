#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Hardware/Pci/pci_event.h>
#include <LibFK/Container/circular_buffer.h>

namespace fkernel {

class PCIDeviceNode final : public CharacterDevice {
public:
    PCIDeviceNode();
    virtual ~PCIDeviceNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::PermissionDenied;
    }
    virtual size_t size() const override;

    void push_event(const PCIEvent& event);

private:
    fk::containers::CircularBuffer<PCIEvent, 64> m_events;
};

} // namespace fkernel
