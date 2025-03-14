#include "../../../../Include/Kernel/Arch/Descriptor/idt.h"
#include "../../../../Include/Kernel/Arch/Descriptor/exception_handler.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

static void idt_set_gate(int n, uint64_t handler, uint16_t selector,
                         uint8_t type_attr, uint8_t ist) {
  idt[n].offset_low = handler & 0xFFFF;
  idt[n].selector = selector;
  idt[n].ist = ist & 0x07;
  idt[n].type_attr = type_attr;
  idt[n].offset_mid = (handler >> 16) & 0xFFFF;
  idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
  idt[n].reserved = 0;
}

void init_idt() {
  print_str("Init IDT\n");
  idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
  idtp.base = (uint64_t)&idt;

  idt_set_gate(0, (uint64_t)division_by_zero_handler, 0x08, 0x8E, 0);
  idt_set_gate(6, (uint64_t)invalid_opcode_handler, 0x08, 0x8E, 0);
  for (int i = 1; i < MAX_EXCEPTIONS; ++i) {
    if (i != 6) {
      idt_set_gate(i, (uint64_t)generic_handler, 0x08, 0x8E, 0);
    }
  }

  idt_flush((uint64_t)&idtp);
  print_str("IDT Loaded\n");
}
