#include "Kernel/Arch/x86_64/Hw/Io.h"
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <LibFK/Log.h>

namespace idt {

static IrqHandler irq_handlers[16] = { nullptr };

void Manager::initialize() noexcept
{
    Log(LogLevel::INFO, "IDT: Initialize Interrupt Descriptor Table from x86_64 (64 Bits)");

    idt::register_irq_handler(0, timer_handler);
    idt::register_irq_handler(1, keyboard_handler);
    idt::register_irq_handler(2, cascade_handler);
    idt::register_irq_handler(3, com2_handler);
    idt::register_irq_handler(4, com1_handler);
    idt::register_irq_handler(5, legacy_peripheral_handler);

    idt::register_irq_handler(6, fdc_handler);
    idt::register_irq_handler(7, spurious_irq7_handler);
    idt::register_irq_handler(8, rtc_handler);
    idt::register_irq_handler(9, acpi_handler);
    idt::register_irq_handler(10, irq10_pci_handler);
    idt::register_irq_handler(11, irq11_pci_handler);
    idt::register_irq_handler(12, ps2_mouse_handler);
    idt::register_irq_handler(13, fpu_handler);
    idt::register_irq_handler(14, primary_ata_handler);
    idt::register_irq_handler(15, secondary_ata_handler);

    for (int i = 0; i < 256; ++i) {
        if (i <= 31) {
            Logf(LogLevel::TRACE, "IDT: Registered Exception %d (%s)", i, named_exception(i));
            set_entry(i, reinterpret_cast<void*>(exception_stubs[i]), 0x08, IDT_TYPE_INTERRUPT_GATE, 0);
        }

        else if (i > 31 && i <= 47) {

            int irq = i - 32;
            Logf(LogLevel::TRACE, "IDT: Registered Routine %d (%s)", i, named_irq(irq));
            set_entry(i, reinterpret_cast<void*>(routine_stubs[irq]), 0x08, IDT_TYPE_INTERRUPT_GATE, 0);
        }

        else {
            //  Logf(LogLevel::TRACE, "Implement a custom handling for index %d", i);
        }
    }

    idtr.limit = sizeof(entries_) - 1;
    idtr.base = reinterpret_cast<LibC::uint64_t>(&entries_[0]);

    flush_idt(&idtr);
    Log(LogLevel::INFO, "IDT: Loaded with success");
}

void Manager::set_entry(int index, void* isr, LibC::uint16_t selector, LibC::uint8_t type_attr, LibC::uint8_t ist) noexcept
{
    auto& entry = entries_[index];
    LibC::uint64_t addr = reinterpret_cast<LibC::uint64_t>(isr);

    entry.offset_low = addr & 0xFFFF;
    entry.selector = selector;
    entry.ist = ist & 0x07; // 3 bits
    entry.type_attr = type_attr;
    entry.offset_mid = (addr >> 16) & 0xFFFF;
    entry.offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry.zero = 0;

    //   Logf(LogLevel::TRACE,
    //       "[IDT] Entry[%d]: isr=0x%016llx sel=0x%04x attr=0x%02x ist=%u",
    //      index, addr, selector, type_attr, ist);
}

void register_irq_handler(LibC::uint8_t irq, IrqHandler handler) noexcept
{
    if (irq >= 16) {
        Logf(LogLevel::ERROR, "IDT: Attempt to register handler for invalid IRQ %u", irq);
        return;
    }

    if (irq_handlers[irq] != nullptr) {
        Logf(LogLevel::WARN, "IDT: Overwriting existing handler for IRQ %u (%s)", irq, named_irq(irq));
    }

    irq_handlers[irq] = handler;
    Logf(LogLevel::INFO, "IDT: Handler registered for IRQ %u (%s)", irq, named_irq(irq));
}

void unregister_irq_handler(LibC::uint8_t irq) noexcept
{
    if (irq >= 16) {
        Logf(LogLevel::ERROR, "IDT: Attempt to unregister handler for invalid IRQ %u", irq);
        return;
    }

    irq_handlers[irq] = nullptr;
    Logf(LogLevel::INFO, "IDT: Handler removed for IRQ %u (%s)", irq, named_irq(irq));
}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context) noexcept
{
    if (irq >= 16) {
        Logf(LogLevel::ERROR, "IRQ_DISPATCH: Invalid IRQ %u", irq);
        Io::send_eoi(irq);
        return;
    }

    if (idt::irq_handlers[irq]) {
        idt::irq_handlers[irq](irq, context);
    } else {
        Logf(LogLevel::WARN, "IRQ_DISPATCH: Unhandled IRQ %u (%s)", irq, named_irq(irq));
    }

    if (irq >= 8) {
        Io::outb(0xA0, 0x20); // EOI para o slave
    }
    Io::outb(0x20, 0x20); // EOI para o master
}
}
