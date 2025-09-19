#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <LibFK/log.h>

idt::idt() {
  clear();
  klog("IDT", "IDT initialized.");
}

void idt::clear() {
  for (size_t i = 0; i < MAX_X86_64_IDT_SIZE; ++i) {
    entries[i] = {};
    isr_handlers[i] = nullptr;
  }

  klog("IDT", "All Idt entries are cleaned");
}

void idt::set_gate(uint8_t vector, void (*handler_ptr)(), uint16_t selector,
                   uint8_t ist, uint8_t type_attr) {
  const uint64_t handler = reinterpret_cast<uint64_t>(handler_ptr);
  Idt_Entry &d = entries[vector];
  d.offset_low = static_cast<uint16_t>(handler & 0xFFFFu);
  d.selector = selector;
  d.ist = static_cast<uint8_t>(ist & 0x7u);
  d.type_attr = type_attr;
  d.offset_mid = static_cast<uint16_t>((handler >> 16) & 0xFFFFu);
  d.offset_high = static_cast<uint32_t>((handler >> 32) & 0xFFFFFFFFu);
  d.zero = 0;

  klog("IDT", "Gate set for vector %u, handler %p", vector, handler_ptr);
}

void idt::register_isr_handler(uint8_t vector, isr_handler_t handler) {
  isr_handlers[vector] = handler;

  klog("IDT", "ISR handler registered for vector %u, handler %p", vector,
       handler);
}
isr_handler_t idt::get_isr_handler(uint8_t vec) const {
  return isr_handlers[vec];
}

void idt::register_irq_handler(uint8_t vector, irq_handler_t handler) {
  irq_handlers[vector] = handler;
  klog("IDT", "IRQ handler registered for vector %u, handler %p", vector,
       handler);
}

irq_handler_t idt::get_irq_handler(uint8_t vec) const {
  return irq_handlers[vec];
}

void idt::load() {
  Idt_ptr ptr;
  ptr.limit = static_cast<uint16_t>(sizeof(entries) - 1);
  ptr.base = reinterpret_cast<uint64_t>(&entries);
  flush_idt(&ptr);

  klog("IDT", "IDT loaded successfully.");
}

Idt_Entry *idt::raw_entries() { return entries.begin(); }
const Idt_Entry *idt::raw_entries() const { return entries.begin(); };
