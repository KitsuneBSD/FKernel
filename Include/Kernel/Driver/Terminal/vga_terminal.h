#pragma once

#include <Kernel/Driver/Terminal/terminal.h>
#include <LibFK/Core/Result.h>

namespace fkernel {
namespace terminal {

// Forward declarations
class InputDevice;
class OutputDevice;

/// @brief VGA Terminal implementation using VGA adapter and PS/2 keyboard
class VGATerminal final : public Terminal {
public:
    explicit VGATerminal(int index);
    virtual ~VGATerminal() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
    virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override;
    virtual size_t size() const override { return 0; }
    
    // Terminal interface
    virtual fk::core::Result<void, fk::core::Error> attach_input(InputDevice* device) override;
    virtual fk::core::Result<void, fk::core::Error> attach_output(OutputDevice* device) override;
    virtual TerminalCapabilities capabilities() const override;
    virtual fk::core::Result<void, fk::core::Error> set_size(uint16_t rows, uint16_t cols) override;
    virtual void get_size(uint16_t& rows, uint16_t& cols) const override;
    virtual const char* type_name() const override;
    
    // Public access to index
    int index() const { return m_index; }

private:
    [[maybe_unused]] int m_index;
    
    // Simple line buffering
    static constexpr size_t LINE_BUFFER_SIZE = 1024;
    char m_line_buffer[LINE_BUFFER_SIZE];
    size_t m_line_len = 0;
    size_t m_read_index = 0;

    // Terminal state
    bool m_raw_mode{false};
    uint16_t m_rows{25};
    uint16_t m_cols{80};
};

} // namespace terminal
} // namespace fkernel