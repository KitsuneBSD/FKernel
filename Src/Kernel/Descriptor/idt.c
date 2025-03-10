#include "../../../Include/Kernel/Descriptor/idt.h"
#include "../../../Include/Kernel/Driver/vga_buffer.h"

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
  idt_entry_t *descriptor = &idt[vector];

  descriptor->isr_low = (uint64_t)isr & 0xFFFF;
  descriptor->kernel_cs = GDT_OFFSET_KERNEL_CODE;
  descriptor->ist = 0;
  descriptor->attributes = flags;
  descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->reserved = 0;
}

void init_idt() {
  print_str("Init IDT\n");

  idtr.base = (uint64_t)&idt[0];
  idtr.limit = (uint16_t)((sizeof(idt_entry_t) * IDT_MAX_ENTRIES) - 1);

  for (uint16_t vector = 0; vector < IDT_MAX_ENTRIES; ++vector) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x0E);
  }

  asm volatile("lidt %0" : : "m"(idtr));
  print_str("IDT Initialized\n");
}

__attribute__((noreturn)) void exception_handler(void) {
  asm volatile("cli; hlt");
  __builtin_unreachable();
}
