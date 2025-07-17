#include "Kernel/Arch/x86_64/Interrupts/Routines.h"
#include "LibFK/enforce.h"
#include <Kernel/Arch/x86_64/Interrupts/Isr.h>
#include <Kernel/Driver/8259Pic.h>
#include <LibFK/log.h>

void Pic8259::remap(int offset1, int offset2) noexcept
{
    FK::enforcef(offset1 >= 0x20 && offset1 <= 0xF8, "PIC8259: Invalid master offset: 0x%X", offset1);
    FK::enforcef(offset2 >= 0x20 && offset2 <= 0xF8, "PIC8259: Invalid slave offset: 0x%X", offset2);

    Logf(LogLevel::INFO, "PIC8259: Remapping PIC — Master=0x%X, Slave=0x%X", offset1, offset2);

    LibC::uint8_t const mask1 = Io::inb(PIC1_DATA);
    LibC::uint8_t const mask2 = Io::inb(PIC2_DATA);

    Io::outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    Io::outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    Io::outb(PIC1_DATA, static_cast<LibC::uint8_t>(offset1));
    Io::outb(PIC2_DATA, static_cast<LibC::uint8_t>(offset2));

    Io::outb(PIC1_DATA, 0x04); // Tell Master PIC that there is a slave PIC at IRQ2
    Io::outb(PIC2_DATA, 0x02); // Tell Slave PIC its cascade identity

    Io::outb(PIC1_DATA, ICW4_8086);
    Io::outb(PIC2_DATA, ICW4_8086);

    Io::outb(PIC1_DATA, mask1); // Restore saved masks
    Io::outb(PIC2_DATA, mask2);

    Log(LogLevel::INFO, "PIC8259: Remap completed successfully");
}

void Pic8259::send_eoi(LibC::uint8_t irq) noexcept
{
    FK::alert_if_f(irq > 15, "PIC8259: send_eoi called with invalid IRQ %u", irq);

    if (irq >= 8)
        Io::outb(PIC2_CMD, 0x20); // EOI to slave

    Io::outb(PIC1_CMD, 0x20); // EOI to master
}

void Pic8259::mask_irq(LibC::uint8_t irq_line) noexcept
{
    FK::enforcef(irq_line < 16, "PIC8259: Cannot mask invalid IRQ line %u", irq_line);

    if (irq_line < 8) {
        auto const old_mask = Io::inb(PIC1_DATA);
        auto const new_mask = old_mask | (1 << irq_line);
        Io::outb(PIC1_DATA, new_mask);
        Logf(LogLevel::TRACE, "PIC8259: Masked IRQ %u (%s) [0x21] — old=0x%02X new=0x%02X",
            irq_line, named_irq(irq_line), old_mask, new_mask);
    } else {
        LibC::uint8_t irq = irq_line - 8;
        auto const old_mask = Io::inb(PIC2_DATA);
        auto const new_mask = old_mask | (1 << irq);
        Io::outb(PIC2_DATA, new_mask);
        Logf(LogLevel::TRACE, "PIC8259: Masked IRQ %u (%s) [0xA1] — old=0x%02X new=0x%02X",
            irq_line, named_irq(irq_line), old_mask, new_mask);
    }
}

void Pic8259::unmask_irq(LibC::uint8_t irq_line) noexcept
{
    FK::enforcef(irq_line < 16, "PIC8259: Cannot unmask invalid IRQ line %u", irq_line);

    LibC::uint16_t const port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    LibC::uint8_t const irq = (irq_line < 8) ? irq_line : irq_line - 8;

    LibC::uint8_t const old_mask = Io::inb(port);
    LibC::uint8_t const new_mask = old_mask & ~(1 << irq);

    Io::outb(port, new_mask);

    Logf(LogLevel::TRACE, "PIC8259: Unmasked IRQ %u (%s) [0x%X] — old=0x%02X new=0x%02X",
        irq_line, named_irq(irq_line), port, old_mask, new_mask);
}
