#include "Kernel/Arch/x86_64/Hardware/Io.h"
#include "LibFK/enforce.h"
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Routines.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <Kernel/Driver/8259Pic/8259Pic.h>
#include <LibFK/log.h>

namespace idt {

extern "C" idt::IrqHandler irq_handlers[16];
extern "C" irq_entry_t irq_table[];

void Manager::validate_index(int index) const noexcept
{
    if (FK::alert_if_f(index < 0 || index >= MAX_IDT_ENTRIES,
            "IDT: index %d out of bounds [0..%d]", index, MAX_IDT_ENTRIES - 1))
        return;
}

void Manager::validate_irq(int irq) const noexcept
{
    if (FK::alert_if_f(irq < 0 || irq >= MAX_IRQ,
            "IDT: IRQ %d out of bounds [0..%d]", irq, MAX_IRQ - 1))
        return;
}

void Manager::set_entry(int index, void* isr, LibC::uint16_t selector, LibC::uint8_t type_attr, LibC::uint8_t ist) noexcept
{
    if (FK::alert_if_f(index < 0 || index >= MAX_IDT_ENTRIES,
            "IDT: set_entry: index %d out of bounds", index))
        return;

    idt_entry& entry = entries_[index];
    LibC::uint64_t addr = reinterpret_cast<LibC::uint64_t>(isr);

    entry.offset_low = addr & 0xFFFF;
    entry.selector = selector;
    entry.ist = ist & IST_MASK;
    entry.type_attr = type_attr;
    entry.offset_mid = (addr >> 16) & 0xFFFF;
    entry.offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry.zero = 0;
}

void Manager::register_exception(int vector) noexcept
{
    if (FK::alert_if_f(vector < 0 || vector > 31,
            "IDT: Exception vector %d out of valid range [0..31]", vector))
        return;

    set_entry(vector, reinterpret_cast<void*>(exception_stubs[vector]),
        KERNEL_CODE_SELECTOR, IDT_TYPE_INTERRUPT_GATE, isr_ist[vector]);

    Logf(LogLevel::TRACE,
        "Register Exception Handler: Handler registered for Exception %u (%s)",
        vector, named_exception(vector));
}

void Manager::register_irq(int irq) noexcept
{
    if (FK::alert_if_f(irq < 0 || irq >= MAX_IRQ,
            "IDT: register_irq: IRQ %d out of bounds", irq))
        return;

    int vector = irq + IRQ_VECTOR_BASE;

    set_entry(vector, reinterpret_cast<void*>(routine_stubs[irq]),
        KERNEL_CODE_SELECTOR, IDT_TYPE_INTERRUPT_GATE, 0);

    register_irq_handler(irq_table[irq].irq, irq_table[irq].handler);
    Pic8259::Instance().mask_irq(irq);
}

void Manager::send_eoi(int irq) noexcept
{
    if (FK::alert_if_f(irq < 0 || irq >= MAX_IRQ,
            "IDT: send_eoi: IRQ %d out of bounds", irq))
        return;

    Pic8259::Instance().send_eoi(irq);
}

void Manager::enable_irqs() noexcept
{
    Pic8259::Instance().unmask_irq(0);
    Pic8259::Instance().unmask_irq(1);
}

void Manager::initialize() noexcept
{
    constexpr int expected_entries = MAX_IDT_ENTRIES;
    Log(LogLevel::INFO, "IDT: Initializing Interrupt Descriptor Table for x86_64 (64-bit)");

    Pic8259::Instance().remap(IRQ_VECTOR_BASE, IRQ_VECTOR_BASE + MAX_IRQ);

    if (FK::alert_if_f(expected_entries != static_cast<int>(sizeof(entries_) / sizeof(entries_[0])),
            "IDT: entries_ array size mismatch"))
        return;

    asm volatile("cli");

    for (int vector = 0; vector <= 31; ++vector)
        register_exception(vector);

    for (int irq = 0; irq < MAX_IRQ; ++irq)
        register_irq(irq);

    idtr.limit = sizeof(entries_) - 1;
    idtr.base = reinterpret_cast<LibC::uint64_t>(&entries_[0]);

    flush_idt(&idtr);
    asm volatile("sti");

    enable_irqs();

    Log(LogLevel::INFO, "IDT: Loaded successfully");
}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context) noexcept
{
    if (FK::alert_if_f(irq >= MAX_IRQ, "IRQ Dispatch: Invalid IRQ number %u", irq)) {
        Pic8259::Instance().send_eoi(irq);
        return;
    }

    if (irq_handlers[irq]) {
        irq_handlers[irq](irq, context);
    } else {
        Logf(LogLevel::WARN, "IRQ Dispatch: Unhandled IRQ %u (%s)", irq, named_irq(irq));
    }

    Pic8259::Instance().send_eoi(irq);
}

}
