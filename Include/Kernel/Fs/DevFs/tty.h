#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class TTYDevice final : public CharacterDevice {
public:
    explicit TTYDevice(int index);
    virtual ~TTYDevice() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual size_t size() const override { return 0; }

private:
    int m_index;
};

} // namespace fkernel
