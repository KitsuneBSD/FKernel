#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <LibC/stdio.h>

idt::idt() { clear(); }

void idt::clear() {
  for (size_t i = 0; i < MAX_X86_64_IDT_SIZE; ++i) {
    entries[i] = {};
    handlers[i] = nullptr;
  }
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
}

void idt::register_handler(uint8_t vector, isr_handler_t handler) {
  handlers[vector] = handler;
}

void idt::load() {
  Idt_ptr ptr;
  ptr.limit = static_cast<uint16_t>(sizeof(entries) - 1);
  ptr.base = reinterpret_cast<uint64_t>(&entries);
  flush_idt(&ptr);
}

isr_handler_t idt::get_handler(uint8_t vec) const { return handlers[vec]; }

Idt_Entry *idt::raw_entries() { return entries.begin(); }
const Idt_Entry *idt::raw_entries() const { return entries.begin(); };
