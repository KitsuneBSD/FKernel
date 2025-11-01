#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <Kernel/Arch/x86_64/Interrupt/non_maskable_interrupt.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/8259_pic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>

#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <LibFK/Algorithms/log.h>

extern "C" void flush_idt(void *idtr);

void InterruptController::initialize() {
    disable_interrupt();

    clear();

    kdebug("INTERRUPT", "Setting up IDT entries...");
    for (size_t i = 0; i < MAX_x86_64_IDT_SIZE; ++i) {
        uint8_t multiple_ist_selector = (i % 7) + 1;
        set_gate(i, g_isr_stubs[i], 0x08, *ist_stacks[multiple_ist_selector]);
        register_interrupt(default_handler, i);
    }

    register_interrupt(nmi_handler, 2);
    register_interrupt(general_protection_handler, 13);
    register_interrupt(page_fault_handler, 14);
    register_interrupt(timer_handler, 32);
    register_interrupt(keyboard_handler, 33);

    load();

    PIC8259::initialize();

    NMI::enable_nmi();

    PIT::the().initialize(100);

    kdebug("PIC", "Unmasking IRQ0 (timer) and IRQ1 (keyboard)");
    PIC8259::unmask_irq(0);
    PIC8259::unmask_irq(1);

    enable_interrupt();
    klog("INTERRUPT", "Interrupt descriptor table initialized");
}

void InterruptController::clear() {
    kdebug("INTERRUPT", "Clearing all IDT entries and handlers");
    for (size_t i = 0; i < MAX_x86_64_IDT_SIZE; ++i) {
        m_entries[i] = {};
        m_handlers[i] = nullptr;
    }
}


void InterruptController::set_gate(uint8_t vector, void (*new_interrupt)(),
                                   uint16_t selector, uint8_t ist,
                                   uint8_t type_attr) {
    const uint64_t handler = reinterpret_cast<uint64_t>(new_interrupt);
    idt_entry &d = m_entries[vector];
    d.offset_low = static_cast<uint16_t>(handler & 0xFFFFu);
    d.selector = selector;
    d.ist = static_cast<uint8_t>(ist & 0x7u);
    d.type_attr = type_attr;
    d.offset_mid = static_cast<uint16_t>((handler >> 16) & 0xFFFFu);
    d.offset_high = static_cast<uint32_t>((handler >> 32) & 0xFFFFFFFFu);
    d.zero = 0;
}

void InterruptController::register_interrupt(interrupt new_interrupt,
                                             uint8_t vector) {
  m_handlers[vector] = new_interrupt;
}

void InterruptController::load() {
    idt_ptr ptr;
    ptr.limit = static_cast<uint16_t>(sizeof(m_entries) - 1);
    ptr.base = reinterpret_cast<uint64_t>(&m_entries);
    flush_idt(&ptr);

    kdebug("INTERRUPT", "IDT loaded to CPU", ptr.base, ptr.limit);
}

interrupt InterruptController::get_interrupt(uint8_t vector) {
  return m_handlers[vector];
}
