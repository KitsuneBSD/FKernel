#include <Kernel/Arch/x86_64/Hardware/Io.h>

namespace Io {

void outb(LibC::uint16_t port, LibC::uint8_t val) noexcept
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint8_t inb(LibC::uint16_t port) noexcept
{
    LibC::uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outw(LibC::uint16_t port, LibC::uint16_t val) noexcept
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint16_t inw(LibC::uint16_t port) noexcept
{
    LibC::uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outl(LibC::uint16_t port, LibC::uint32_t val) noexcept
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

LibC::uint32_t inl(LibC::uint16_t port) noexcept
{
    LibC::uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void send_eoi(int irq_number) noexcept
{
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
