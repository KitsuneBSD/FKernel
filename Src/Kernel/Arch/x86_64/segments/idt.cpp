#include "Arch/x86_64/irq.h"
#include "Arch/x86_64/irq_handler.hpp"
#include <Kernel/Arch/x86_64/global_exception_handler.h>
#include <Kernel/Arch/x86_64/idt.h>
#include <Kernel/Arch/x86_64/irq.h>
#include <Kernel/Arch/x86_64/stack_size.h>
#include <LibFK/Log.h>

alignas(0x10) IDTEntry idt[IDT_ENTRIES];
IDTPointer idtp;

void set_idt_entry(size_t vector, void (*handler)(), uint8_t ist,
                   uint8_t flags) {
  uint64_t addr = reinterpret_cast<uint64_t>(handler);
  idt[vector].offset_low = addr & 0xFFFF;
  idt[vector].selector = 0x08; // Kernel code segment
  idt[vector].ist = ist & 0x7;
  idt[vector].type_attr = flags;
  idt[vector].offset_middle = (addr >> 16) & 0xFFFF;
  idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vector].zero = 0;
}

void install_irq_handlers() {
  constexpr uint8_t FLAGS = 0x8E;
  remap(0x20, 0x28);

  for (int i = 0; i < 16; ++i) {
    set_idt_entry(0x20 + i, irq_stubs[i], 0, FLAGS);
  }

  Logf(LogLevel::INFO, "IRQ handlers installed into IDT.");
}

void init_idt_entries() {
  Logf(LogLevel::INFO, "Populating IDT with default ISR handlers...");

  for (size_t i = 0; i < IDT_ENTRIES; ++i) {
    set_idt_entry(i, isr_handlers[i], isr_ist[i], IDT_INTERRUPT_GATE_FLAGS);
  }
}

void init_idt() {
  asm volatile("cli");
  Logf(LogLevel::INFO, "Starting IDT initialization...");

  init_idt_entries();
  init_exception_handlers();
  install_irq_handlers();

  register_irq_handler(0, timer_handler);

  idtp.limit = sizeof(IDTEntry) * IDT_ENTRIES - 1;
  idtp.base = reinterpret_cast<uint64_t>(&idt);

  Logf(LogLevel::INFO,
       "IDT pointer set at base 0x%lx, limit %u. Loading IDT with lidt...",
       idtp.base, idtp.limit);

  flush_idt(&idtp);

  Logf(LogLevel::INFO, "IDT loaded successfully.");
  asm volatile("sti");
}
