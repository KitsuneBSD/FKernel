#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <LibFK/enforce.h>

namespace Io {

void outb(LibC::uint16_t port, LibC::uint8_t val) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "outb: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint8_t inb(LibC::uint16_t port) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "inb: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    LibC::uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outw(LibC::uint16_t port, LibC::uint16_t val) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "outw: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint16_t inw(LibC::uint16_t port) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "inw: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    LibC::uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outl(LibC::uint16_t port, LibC::uint32_t val) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "outl: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint32_t inl(LibC::uint16_t port) noexcept
{
    FK::enforcef(port <= X86_64_IO_PORT_MAX_INDEX,
        "inl: port 0x%X out of valid range [0x0000..0xFFFF]", port);

    LibC::uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void send_eoi(int irq_number) noexcept
{
    FK::enforcef(
        irq_number >= 0 && irq_number <= MAX_IRQ_NUMBER,
        "send_eoi: irq_number %d is out of valid range [0..15]",
        irq_number);

    if (irq_number >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void io_wait() noexcept
{
    outb(0x80, 0);
}

}
