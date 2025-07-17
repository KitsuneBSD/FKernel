#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Driver/SerialPort.h>
#include <LibFK/enforce.h>

namespace Serial {

SerialPort& SerialPort::Instance()
{
    static SerialPort instance(Port::COM1); // Altere se quiser outra porta
    return instance;
}

SerialPort::SerialPort(Port port)
    : base_(static_cast<LibC::uint16_t>(port))
{
    FK::enforcef(base_ == 0x3F8 || base_ == 0x2F8 || base_ == 0x3E8 || base_ == 0x2E8,
        "SerialPort: Invalid port base address: 0x%X", base_);
}

void SerialPort::initialize() const
{
    Logf(LogLevel::INFO, "SerialPort: Initializing on base 0x%X", base_);

    Io::outb(base_ + 1, 0x00); // Disable interrupts
    Io::outb(base_ + 3, 0x80); // Enable DLAB (Divisor Latch Access Bit)
    Io::outb(base_ + 0, 0x03); // Divisor LSB (38400 baud)
    Io::outb(base_ + 1, 0x00); // Divisor MSB

    Io::outb(base_ + 3, 0x03); // 8 bits, no parity, one stop bit
    Io::outb(base_ + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
    Io::outb(base_ + 4, 0x0B); // Enable IRQs, set RTS/DSR

    FK::alert_if_f(!(Io::inb(base_ + 5) & 0x20),
        "SerialPort: TX not ready immediately after initialization");

    Log(LogLevel::INFO, "SerialPort: Initialization complete");
}

bool SerialPort::is_transmit_ready() const
{
    LibC::uint8_t const status = Io::inb(base_ + 5);
    return (status & 0x20) != 0;
}

void SerialPort::write_char(char c) const
{
    for (int tries = 0; tries < 1000000; ++tries) {
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
        if (*str == '\n') {
            write_char('\r');
        }
        write_char(*str++);
    }
}

}
