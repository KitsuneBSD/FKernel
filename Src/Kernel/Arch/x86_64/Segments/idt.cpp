#include "Kernel/Arch/x86_64/Hardware/Io.h"
#include "LibFK/enforce.h"
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
    Log(LogLevel::INFO, "IDT: Initializing Interrupt Descriptor Table for x86_64 (64-bit)");

    Pic8259::remap(0x20, 0x28);
    Log(LogLevel::TRACE, "IDT: PIC remapped successfully");

    constexpr int max_entries = sizeof(entries_) / sizeof(entries_[0]);
    FK::enforcef(max_entries >= 256, "IDT: entries_ array too small for 256 entries");

    for (int i = 0; i < 256; ++i) {
        if (i <= 31) {
            Logf(LogLevel::TRACE, "IDT: Registering Exception %d (%s)", i, named_exception(i));
            set_entry(i, reinterpret_cast<void*>(exception_stubs[i]), 0x08, IDT_TYPE_INTERRUPT_GATE, isr_ist[i]);
        } else if (i <= 47) {
            int irq = i - 32;
            FK::enforcef(irq < 16, "IDT: IRQ index %d out of range", irq);
            Logf(LogLevel::TRACE, "IDT: Registering IRQ %d (%s)", i, named_irq(irq));
            set_entry(i, reinterpret_cast<void*>(routine_stubs[irq]), 0x08, IDT_TYPE_INTERRUPT_GATE, 0);
            register_irq_handler(irq_table[irq].irq, irq_table[irq].handler);
            Pic8259::mask_irq(irq);
        }
    }

    idtr.limit = sizeof(entries_) - 1;
    idtr.base = reinterpret_cast<LibC::uint64_t>(&entries_[0]);

    flush_idt(&idtr);
    asm volatile("sti");

    Pic8259::unmask_irq(0);
    Pic8259::unmask_irq(1);

    Log(LogLevel::INFO, "IDT: Loaded successfully");
}

void Manager::set_entry(int index, void* isr, LibC::uint16_t selector, LibC::uint8_t type_attr, LibC::uint8_t ist) noexcept
{
    constexpr int max_entries = sizeof(entries_) / sizeof(entries_[0]);
    FK::enforcef(index >= 0 && index < max_entries,
        "IDT: set_entry index %d out of bounds [0..%d]", index, max_entries - 1);

    auto& entry = entries_[index];
    LibC::uint64_t addr = reinterpret_cast<LibC::uint64_t>(isr);

    entry.offset_low = addr & 0xFFFF;
    entry.selector = selector;
    entry.ist = ist & 0x07; // 3 bits only
    entry.type_attr = type_attr;
    entry.offset_mid = (addr >> 16) & 0xFFFF;
    entry.offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry.zero = 0;

    Logf(LogLevel::TRACE, "IDT: Entry[%d] ISR=0x%016llx sel=0x%04x attr=0x%02x ist=%u",
        index, addr, selector, type_attr, ist);
}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context) noexcept
{
    if (irq >= 16) {
        Logf(LogLevel::ERROR, "IRQ Dispatch: Invalid IRQ number %u", irq);
        Io::send_eoi(irq); // Defensive EOI even if invalid IRQ
        return;
    }

    if (irq_handlers[irq]) {
        irq_handlers[irq](irq, context);
    } else {
        Logf(LogLevel::WARN, "IRQ Dispatch: Unhandled IRQ %u (%s)", irq, named_irq(irq));
    }

    if (irq >= 8) {
        Io::outb(0xA0, 0x20); // Slave PIC
    }
    Io::outb(0x20, 0x20); // Master PIC
}
}
