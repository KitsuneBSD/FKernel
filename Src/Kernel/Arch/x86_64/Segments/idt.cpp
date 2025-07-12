#include "Kernel/Arch/x86_64/Hardware/Io.h"
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <Kernel/Driver/8259Pic.h>
#include <LibFK/log.h>

namespace idt {

extern "C" idt::IrqHandler irq_handlers[16];
extern "C" irq_entry_t irq_table[];

void Manager::initialize() noexcept
{
    Log(LogLevel::INFO, "IDT: Initialize Interrupt Descriptor Table from x86_64 (64 Bits)");

    Pic8259::remap(0x20, 0x28);

    Log(LogLevel::TRACE, "IDT: PIC remapped successfully");

    for (int i = 0; i < 256; ++i) {
        if (i <= 31) {
            Logf(LogLevel::TRACE, "IDT: Registered Exception %d (%s)", i, named_exception(i));
            set_entry(i, reinterpret_cast<void*>(exception_stubs[i]), 0x08, IDT_TYPE_INTERRUPT_GATE, isr_ist[i]);
        }

        else if (i > 31 && i <= 47) {

            int irq = i - 32;
            Logf(LogLevel::TRACE, "IDT: Registered Routine %d (%s)", i, named_irq(irq));
            set_entry(i, reinterpret_cast<void*>(routine_stubs[irq]), 0x08, IDT_TYPE_INTERRUPT_GATE, 0);
            register_irq_handler(irq_table[irq].irq, irq_table[irq].handler);
            Pic8259::mask_irq(irq);
        }

        else {
            //  Logf(LogLevel::TRACE, "Implement a custom handling for index %d", i);
        }
    }
    idtr.limit = sizeof(entries_) - 1;
    idtr.base = reinterpret_cast<LibC::uint64_t>(&entries_[0]);

    flush_idt(&idtr);
    asm volatile("sti");

    Pic8259::unmask_irq(0);
    Pic8259::unmask_irq(1);
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
extern "C" void irq_dispatch(LibC::uint8_t irq, void* context) noexcept
{
    if (irq >= 16) {
        Logf(LogLevel::ERROR, "Dispatch: Invalid IRQ %u", irq);
        Io::send_eoi(irq);
        return;
    }

    if (irq_handlers[irq]) {
        irq_handlers[irq](irq, context);
    } else {
        Logf(LogLevel::WARN, "Dispatch: Unhandled IRQ %u (%s)", irq, named_irq(irq));
    }

    if (irq >= 8) {
        Io::outb(0xA0, 0x20); // EOI para o slave
    }
    Io::outb(0x20, 0x20); // EOI para o master
}
}
