#pragma once

#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_frame.h>
#include <Kernel/Arch/x86_64/Interrupt/isr_stubs.h>
#include <Kernel/Arch/x86_64/asm.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/stdio.h>
#include <LibFK/array.h>

using isr_handler_t = void (*)(InterruptFrame *frame, uint8_t vector);

class IDT {
private:
  array<Idt_Entry, MAX_X86_64_IDT_SIZE> entries;
  isr_handler_t handlers[MAX_X86_64_IDT_SIZE];

public:
  IDT() { clear(); }

  void clear() {
    for (size_t i = 0; i < MAX_X86_64_IDT_SIZE; ++i) {
      entries[i] = {};
      handlers[i] = nullptr;
    }
  }

  void set_gate(uint8_t vector, void (*handler_ptr)(), uint16_t selector = 0x08,
                uint8_t ist = 0, uint8_t type_attr = 0x8E) {
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

  void register_handler(uint8_t vector, isr_handler_t handler) {
    handlers[vector] = handler;
  }

  void load() {
    Idt_ptr ptr;
    ptr.limit = static_cast<uint16_t>(sizeof(entries) - 1);
    ptr.base = reinterpret_cast<uint64_t>(&entries);
    flush_idt(&ptr);
  }

  isr_handler_t get_handler(uint8_t vec) const { return handlers[vec]; }

  static void default_handler([[maybe_unused]] InterruptFrame *frame,
                              [[maybe_unused]] uint8_t vector) {
    kprintf("Unhandled interrupt: vector=%u\n", (unsigned)vector);
    for (;;) {
      asm volatile("hlt");
    }
  }

  Idt_Entry *raw_entries() { return entries.begin(); }
  const Idt_Entry *raw_entries() const { return entries.begin(); }
};

void init_idt();
