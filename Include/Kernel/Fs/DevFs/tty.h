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
    [[maybe_unused]] int m_index;
    
    // Simple line buffering
    static constexpr size_t LINE_BUFFER_SIZE = 1024;
    char m_line_buffer[LINE_BUFFER_SIZE];
    size_t m_line_len = 0;
    size_t m_read_index = 0;
};

} // namespace fkernel
