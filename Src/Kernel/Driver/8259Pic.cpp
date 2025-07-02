#include "Kernel/Arch/x86_64/Interrupts/Routines.h"
#include <Kernel/Arch/x86_64/Interrupts/Isr.h>
#include <Kernel/Driver/8259Pic.h>
#include <LibFK/log.h>

void Pic8259::remap(int offset1, int offset2) noexcept
{

    Logf(LogLevel::INFO, "PIC8259: Remapping PIC with offsets: Master=%x, Slave=%x", offset1, offset2);
    LibC::uint8_t const mask1 = Io::inb(PIC1_DATA);
    LibC::uint8_t const mask2 = Io::inb(PIC2_DATA);

    Io::outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    Io::outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    Io::outb(PIC1_DATA, static_cast<LibC::uint8_t>(offset1));
    Io::outb(PIC2_DATA, static_cast<LibC::uint8_t>(offset2));

    Io::outb(PIC1_DATA, 0x04); // Master tem slave na IRQ2
    Io::outb(PIC2_DATA, 0x02); // Slave está na IRQ2 do master

    Io::outb(PIC1_DATA, ICW4_8086);
    Io::outb(PIC2_DATA, ICW4_8086);

    Io::outb(PIC1_DATA, mask1);
    Io::outb(PIC2_DATA, mask2);

    Log(LogLevel::INFO, "PIC8259: Remap complete");
}

void Pic8259::send_eoi(LibC::uint8_t irq) noexcept
{
    if (irq >= 8) {
        Io::outb(PIC2_CMD, 0x20);
    }
    Io::outb(PIC1_CMD, 0x20);
}

void Pic8259::mask_irq(LibC::uint8_t irq_line) noexcept
{
    if (irq_line < 8) {
        auto old_mask = Io::inb(PIC1_DATA);
        Io::outb(PIC1_DATA, old_mask | (1 << irq_line));
        Logf(LogLevel::TRACE, "PIC8259: Masked IRQ line %u (%s) (port 0x21) (old_mask=0x%02X new_mask=0x%02X)",
            irq_line, named_irq(irq_line), old_mask, old_mask | (1 << irq_line));
    } else {
        irq_line -= 8;
        auto old_mask = Io::inb(PIC2_DATA);
        Io::outb(PIC2_DATA, old_mask | (1 << irq_line));
        Logf(LogLevel::TRACE, "PIC8259: Masked IRQ line %u (%s) (port 0xA1) (old_mask=0x%02X new_mask=0x%02X)",
            irq_line + 8, named_irq(irq_line + 8), old_mask, old_mask | (1 << irq_line));
    }
}

void Pic8259::unmask_irq(LibC::uint8_t irq_line) noexcept
{
    LibC::uint16_t const port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq_line >= 8)
        irq_line -= 8;

    LibC::uint8_t const old_mask = Io::inb(port);
    LibC::uint8_t const new_mask = old_mask & ~(1 << irq_line);

    Io::outb(port, new_mask);

    Logf(LogLevel::TRACE, "PIC8259: Unmasked IRQ line %u (port 0x%x) (old_mask=0x%02x new_mask=0x%02x)", irq_line, port, old_mask, new_mask);
}
