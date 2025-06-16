#include <Kernel/Arch/x86_64/idt.h>
#include <Kernel/Arch/x86_64/stack_size.h>
#include <LibFK/Log.h>

alignas(0x10) IDTEntry idt[IDT_ENTRIES];
IDTPointer idtp;

void set_idt_entry(int vector, void (*handler)(), uint8_t ist, uint8_t flags) {
  uint64_t addr = reinterpret_cast<uint64_t>(handler);
  idt[vector].offset_low = addr & 0xFFFF;
  idt[vector].selector = 0x08; // Kernel code segment
  idt[vector].ist = ist & 0x7;
  idt[vector].type_attr = flags;
  idt[vector].offset_middle = (addr >> 16) & 0xFFFF;
  idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vector].zero = 0;
}

void init_idt() {
  Logf(LogLevel::INFO, "Initializing Interrupt Descriptor Table (IDT)...");

  for (int i = 0; i < IDT_ENTRIES; ++i) {
    set_idt_entry(i, isr_handlers[i], isr_ist[i], IDT_INTERRUPT_GATE_FLAGS);
  }

  idtp.limit = sizeof(IDTEntry) * 256 - 1; // ainda reserva o total (256)
  idtp.base = reinterpret_cast<uint64_t>(&idt);

  Logf(LogLevel::INFO,
       "IDT pointer constructed at base 0x%lx, limit %u. Executing lidt...",
       idtp.base, idtp.limit);
  flush_idt(&idtp);
  Logf(LogLevel::INFO, "IDT successfully loaded.");
}
