#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Driver/SerialPort.h>

Serial::SerialPort& Serial::SerialPort::Instance()
{
    static SerialPort instance(Port::COM1); // Altere se quiser outra porta
    return instance;
}

Serial::SerialPort::SerialPort(Port port)
    : base_(static_cast<LibC::uint16_t>(port))
{
}

void Serial::SerialPort::initialize() const
{
    Io::outb(base_ + 1, 0x00); // Disable interrupts
    Io::outb(base_ + 3, 0x80); // Enable DLAB
    Io::outb(base_ + 0, 0x03); // Divisor low byte (38400 baud)
    Io::outb(base_ + 1, 0x00); // Divisor high byte
    Io::outb(base_ + 3, 0x03); // 8 bits, no parity, one stop bit
    Io::outb(base_ + 2, 0xC7); // Enable FIFO, clear them, 14-byte threshold
    Io::outb(base_ + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

bool Serial::SerialPort::is_transmit_ready() const
{
    return Io::inb(base_ + 5) & 0x20;
}

void Serial::SerialPort::write_char(char c) const
{
    while (!is_transmit_ready())
        ;
    Io::outb(base_, static_cast<LibC::uint8_t>(c));
}

void Serial::SerialPort::write(char const* str) const
{
    while (*str) {
        if (*str == '\n')
            write_char('\r');
        write_char(*str++);
    }
}
