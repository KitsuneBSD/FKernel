#include "Kernel/Driver/Ata/AtaTypes.h"
#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>
#include <LibFK/types.h>

#include <Kernel/Driver/Ata/AtaController.h>

static LibC::uint64_t tick_count = 0;

idt::IrqHandler irq_handlers[MAX_IRQ_NUMBER] = { nullptr };

irq_entry_t irq_table[MAX_IRQ_NUMBER] = {
    { 0, timer_handler },
    { 1, keyboard_handler },
    { 2, cascade_handler },
    { 3, com2_handler },
    { 4, com1_handler },
    { 5, legacy_peripheral_handler },
    { 6, fdc_handler },
    { 7, spurious_irq7_handler },
    { 8, rtc_handler },
    { 9, acpi_handler },
    { 10, irq10_pci_handler },
    { 11, irq11_pci_handler },
    { 12, ps2_mouse_handler },
    { 13, fpu_handler },
    { 14, primary_ata_handler },
    { 15, secondary_ata_handler }
};

LibC::uint64_t uptime()
{
    return tick_count;
}

extern "C" void (*const routine_stubs[16])() = {
    irq0_handler, irq1_handler, irq2_handler, irq3_handler,
    irq4_handler, irq5_handler, irq6_handler, irq7_handler,
    irq8_handler, irq9_handler, irq10_handler, irq11_handler,
    irq12_handler, irq13_handler, irq14_handler, irq15_handler
};

extern "C" void timer_handler(LibC::uint8_t, void*)
{
    ++tick_count;

    Io::send_eoi(0);
}

extern "C" void keyboard_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    LibC::uint8_t scancode = Io::inb(0x60);

    Logf(LogLevel::TRACE, "Keyboard IRQ - scancode: 0x%02X", scancode);

    Io::send_eoi(1);
}

extern "C" void cascade_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Cascade IRQ2 triggered.");

    Io::send_eoi(2);
}

extern "C" void com2_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Serial COM2 IRQ3 triggered.");

    Io::send_eoi(3);
}

extern "C" void com1_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Serial COM1 IRQ4 triggered.");

    Io::send_eoi(4);
}

extern "C" void legacy_peripheral_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Legacy peripheral IRQ5 triggered.");

    Io::send_eoi(5);
}

extern "C" void fdc_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Floppy Disk Controller IRQ6 triggered.");

    Io::send_eoi(6);
}

extern "C" void spurious_irq7_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::WARN, "IRQ7 triggered — possibly spurious.");

    Io::send_eoi(7);
}

extern "C" void rtc_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Real-Time Clock IRQ8 triggered.");

    Io::send_eoi(8);
}

extern "C" void acpi_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "ACPI / IRQ2 redirected IRQ9 triggered.");

    Io::send_eoi(9);
}

extern "C" void irq10_pci_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "IRQ10 triggered (PCI / livre).");

    Io::send_eoi(10);
}

extern "C" void irq11_pci_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "IRQ11 triggered (PCI / livre).");

    Io::send_eoi(11);
}

extern "C" void ps2_mouse_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "PS/2 Mouse IRQ12 triggered.");

    Io::send_eoi(12);
}

extern "C" void fpu_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "FPU / x87 Coprocessor IRQ13 triggered.");

    Io::send_eoi(13);
}

extern "C" void primary_ata_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Primary ATA IRQ14 triggered.");

    ATAController::instance().handle_irq(ChannelType::Primary);

    Io::send_eoi(14);
}

extern "C" void secondary_ata_handler(LibC::uint8_t irq, void* context)
{
    UNUSED(irq)
    UNUSED(context)

    Logf(LogLevel::TRACE, "Secondary ATA IRQ15 triggered.");
    ATAController::instance().handle_irq(ChannelType::Secondary);
    Io::send_eoi(15);
}

void register_irq_handler(LibC::uint8_t irq, idt::IrqHandler handler) noexcept
{
    FK::enforcef(irq < MAX_IRQ_NUMBER,
        "Register Irq Handler: Invalid IRQ %u, valid range is [0..15]", irq);

    FK::alert_if_f(irq_handlers[irq] != nullptr,
        "Register Irq Handler: Overwriting existing handler for IRQ %u (%s)",
        irq, named_irq(irq));

    irq_handlers[irq] = handler;
    Logf(LogLevel::TRACE, "Register Irq Handler: Handler registered for IRQ %u (%s)", irq, named_irq(irq));
}

void unregister_irq_handler(LibC::uint8_t irq) noexcept
{
    FK::enforcef(irq < MAX_IRQ_NUMBER,
        "Unregister Irq Handler: Invalid IRQ %u, valid range is [0..15]", irq);

    FK::alert_if_f(irq_handlers[irq] == nullptr,
        "Unregister Irq Handler: Removing handler for IRQ %u which was already null (%s)",
        irq, named_irq(irq));

    irq_handlers[irq] = nullptr;
    Logf(LogLevel::TRACE, "Unregister Irq Handler: Handler removed for IRQ %u (%s)", irq, named_irq(irq));
}
