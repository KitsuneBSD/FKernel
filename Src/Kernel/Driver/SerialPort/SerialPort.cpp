#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Driver/SerialPort/SerialPort.h>
#include <Kernel/Driver/SerialPort/SerialPort_constants.h>
#include <LibFK/enforce.h>

namespace Serial {

SerialPort& SerialPort::Instance()
{
    static SerialPort instance(Port::COM1);
    return instance;
}

SerialPort::SerialPort(Port port)
    : base_(static_cast<LibC::uint16_t>(port))
{
    if (FK::alert_if_f(
            base_ != static_cast<LibC::uint16_t>(Port::COM1) && base_ != static_cast<LibC::uint16_t>(Port::COM2) && base_ != static_cast<LibC::uint16_t>(Port::COM3) && base_ != static_cast<LibC::uint16_t>(Port::COM4),
            "SerialPort: Invalid port base address: 0x%X, defaulting to COM1", base_)) {
        base_ = static_cast<LibC::uint16_t>(Port::COM1);
    }
}

void SerialPort::initialize() const
{
    Logf(LogLevel::INFO, "SerialPort: Initializing on base 0x%X", base_);

    Io::outb(base_ + Register::INTERRUPT_ENABLE, 0x00);
    Io::outb(base_ + Register::LINE_CONTROL, LineControl::DLAB);
    Io::outb(base_ + Register::DATA, Defaults::DIVISOR_LSB);
    Io::outb(base_ + Register::INTERRUPT_ENABLE, Defaults::DIVISOR_MSB);
    Io::outb(base_ + Register::LINE_CONTROL, Defaults::LINE_CONTROL);
    Io::outb(base_ + Register::FIFO_CONTROL, Defaults::FIFO_CONTROL);
    Io::outb(base_ + Register::MODEM_CONTROL, Defaults::MODEM_CONTROL);

    FK::alert_if_f(!(Io::inb(base_ + Register::LINE_STATUS) & LineStatus::TRANSMIT_HOLD_EMPTY),
        "SerialPort: TX not ready immediately after initialization");

    Log(LogLevel::INFO, "SerialPort: Initialization complete");
}

bool SerialPort::is_transmit_ready() const
{
    LibC::uint8_t status = Io::inb(base_ + Register::LINE_STATUS);
    return (status & LineStatus::TRANSMIT_HOLD_EMPTY) != 0;
}

void SerialPort::write_char(char c) const
{
    for (int tries = 0; tries < 1'000'000; ++tries) {
        if (is_transmit_ready()) {
            Io::outb(base_, static_cast<LibC::uint8_t>(c));
            return;
        }
    }

    FK::alert_if_f(true, "SerialPort: Timeout while waiting for TX ready");
}

void SerialPort::write(char const* str) const
{
    FK::enforcef(str != nullptr, "SerialPort: Tried to write null string");

    while (*str) {
        if (*str == '\n')
            write_char('\r');
        write_char(*str++);
    }
}

}
