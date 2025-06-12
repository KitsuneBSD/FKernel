#include <Kernel/Arch/x86_64/idt.h>
#include <LibFK/Log.h>

constexpr size_t IDT_ENTRIES = 256;

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

extern "C" void isr_stub_0();

void init_idt() {
  Log(LogLevel::INFO, "Initializing Interrupt Descriptor Table (IDT)...");

  // TODO: Create stubs ISR to all possible exceptions
  // TODO: Create IRQ threament
  // TODO: Update isr_stub_0 to logging and rescue
  for (int i = 0; i < IDT_ENTRIES; ++i) {
    set_idt_entry(i, isr_stub_0, 0, 0x8E);
  }

  idtp.limit = sizeof(IDTEntry) * IDT_ENTRIES - 1;
  idtp.base = reinterpret_cast<uint64_t>(&idt);

  Log(LogLevel::INFO, "IDT pointer constructed. Executing lidt...");
  flush_idt(&idtp);
  Log(LogLevel::INFO, "IDT successfully loaded.");
}
