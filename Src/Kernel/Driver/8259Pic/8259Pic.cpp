#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Hardware/Io.h>
#    include <Kernel/Arch/x86_64/Interrupts/Isr.h>
#    include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#endif

#include <Kernel/Driver/8259Pic/8259Pic.h>
#include <Kernel/Driver/8259Pic/8259Pic_Constants.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

void Pic8259::validate_irq_line(LibC::uint8_t irq_line) const noexcept
{
    if (FK::alert_if_f(irq_line >= 16, "PIC8259: IRQ line %u out of valid range [0..15]", irq_line))
        return;
}

void Pic8259::send_eoi(LibC::uint8_t irq) noexcept
{
    if (FK::alert_if_f(irq > 15, "PIC8259: send_eoi called with invalid IRQ %u", irq))
        return;

    if (irq >= 8)
        send_eoi_slave();

    send_eoi_master();
}

void Pic8259::remap(int master_offset, int slave_offset) noexcept
{
    if (FK::alert_if_f(master_offset < 0x20 || master_offset > 0xF8,
            "PIC8259: Invalid master offset: 0x%X", master_offset))
        return;

    if (FK::alert_if_f(slave_offset < 0x20 || slave_offset > 0xF8,
            "PIC8259: Invalid slave offset: 0x%X", slave_offset))
        return;

    LibC::uint8_t mask1 = Io::inb(PIC1_DATA);
    LibC::uint8_t mask2 = Io::inb(PIC2_DATA);

    Io::outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    Io::outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    Io::outb(PIC1_DATA, static_cast<LibC::uint8_t>(master_offset));
    Io::outb(PIC2_DATA, static_cast<LibC::uint8_t>(slave_offset));

    Io::outb(PIC1_DATA, 0x04); // Master PIC: slave at IRQ2
    Io::outb(PIC2_DATA, 0x02); // Slave PIC: cascade identity

    Io::outb(PIC1_DATA, ICW4_8086);
    Io::outb(PIC2_DATA, ICW4_8086);

    Io::outb(PIC1_DATA, mask1);
    Io::outb(PIC2_DATA, mask2);
}

void Pic8259::send_eoi_master() noexcept
{
    Io::outb(PIC2_CMD, PIC_EOI);
}

void Pic8259::send_eoi_slave() noexcept
{
    Io::outb(PIC1_CMD, PIC_EOI);
}

LibC::uint16_t Pic8259::get_port_for_irq(LibC::uint8_t irq_line) const noexcept
{
    return (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
}

LibC::uint8_t Pic8259::get_irq_mask_bit(LibC::uint8_t irq_line) const noexcept
{
    return (irq_line < 8) ? irq_line : (irq_line - 8);
}

void Pic8259::mask_irq(LibC::uint8_t irq_line) noexcept
{
    validate_irq_line(irq_line);

    LibC::uint16_t port = get_port_for_irq(irq_line);
    LibC::uint8_t bit = get_irq_mask_bit(irq_line);

    LibC::uint8_t old_mask = Io::inb(port);
    LibC::uint8_t new_mask = old_mask | (1 << bit);

    Io::outb(port, new_mask);
}

void Pic8259::unmask_irq(LibC::uint8_t irq_line) noexcept
{
    validate_irq_line(irq_line);

    LibC::uint16_t port = get_port_for_irq(irq_line);
    LibC::uint8_t bit = get_irq_mask_bit(irq_line);

    LibC::uint8_t old_mask = Io::inb(port);
    LibC::uint8_t new_mask = old_mask & ~(1 << bit);

    Io::outb(port, new_mask);
}
