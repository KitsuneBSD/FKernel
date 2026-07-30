#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Hardware/Pci/pci_event.h>
#include <LibFK/Container/circular_buffer.h>
#include <LibFK/Types/types.h>

namespace fkernel {

// ioctl request codes for PCI configuration space access.
static constexpr uint64_t PIOC_READ_CONFIG  = 0x5001UL;
static constexpr uint64_t PIOC_WRITE_CONFIG = 0x5002UL;

// Passed as arg pointer (userspace → kernel via copy_from_user / copy_to_user).
struct PiocConfigOp {
    uint8_t  bus;       // PCI bus number
    uint8_t  dev;       // PCI device number (0–31)
    uint8_t  fn;        // PCI function number (0–7)
    uint8_t  width;     // transfer width: 1, 2, or 4 bytes
    uint16_t offset;    // config space register offset (0–255 for standard)
    uint16_t _pad;
    uint32_t value;     // PIOC_READ_CONFIG: output; PIOC_WRITE_CONFIG: input
};

class PCIDeviceNode final : public CharacterDevice {
public:
    PCIDeviceNode();
    virtual ~PCIDeviceNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::PermissionDenied;
    }
    virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override;
    virtual size_t size() const override;

    void push_event(const PCIEvent& event);

private:
    fk::containers::CircularBuffer<PCIEvent, 64> m_events;
};

} // namespace fkernel
