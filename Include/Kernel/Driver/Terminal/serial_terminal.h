#pragma once

#include <Kernel/Driver/Terminal/terminal.h>
#include <Kernel/Driver/Serial/serial_port.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace terminal {

class SerialTerminal final : public Terminal {
public:
    explicit SerialTerminal(const char* port_config);
    virtual ~SerialTerminal() override = default;

    virtual fk::core::Result<size_t, fk::core::Error>
    read(uint64_t offset, size_t size, uint8_t* buffer) override;

    virtual fk::core::Result<size_t, fk::core::Error>
    write(uint64_t offset, size_t size, const uint8_t* buffer) override;

    virtual fk::core::Result<int, fk::core::Error>
    ioctl(uint64_t request, uint64_t arg) override;

    virtual size_t size() const override { return 0; }

    virtual fk::core::Result<void, fk::core::Error>
    attach_input(InputDevice*) override { return {}; }

    virtual fk::core::Result<void, fk::core::Error>
    attach_output(OutputDevice*) override { return {}; }

    virtual TerminalCapabilities capabilities() const override;
    virtual fk::core::Result<void, fk::core::Error> set_size(uint16_t rows, uint16_t cols) override;
    virtual void get_size(uint16_t& rows, uint16_t& cols) const override;
    virtual const char* type_name() const override { return "serial"; }

private:
    uint16_t m_rows{25};
    uint16_t m_cols{80};
    bool m_raw_mode{false};
    bool m_echo_enabled{true};
    bool m_isig_enabled{true};
    fk::synchronization::Spinlock m_lock;
};

} // namespace terminal
} // namespace fkernel
