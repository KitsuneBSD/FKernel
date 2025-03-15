#include "../../../../Include/Kernel/Arch/Descriptor/exception_handler.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

void log_registers(register_t *regs) {
  print_str("RAX: ");
  print_hex(regs->rax);
  print_str("\n");
  print_str("RBX: ");
  print_hex(regs->rbx);
  print_str("\n");
  print_str("RCX: ");
  print_hex(regs->rcx);
  print_str("\n");
  print_str("RDX: ");
  print_hex(regs->rdx);
  print_str("\n");
  print_str("RIP: ");
  print_hex(regs->rip);
  print_str("\n");
  print_str("CS: ");
  print_hex(regs->cs);
  print_str("\n");
}

void panic(const char *segpoint, register_t *regs) {
  print_str("Kernel panic - ");
  print_str(segpoint);
  print_str("\n");

  log_registers(regs);

  while (1) {
    asm("cli; hlt");
  }
}

void generic_handler(register_t *regs) { panic("Unhandled exception", regs); }

void invalid_opcode_handler(register_t *regs) { panic("Invalid Opcode", regs); }

void division_by_zero_handler(register_t *regs) {
  panic("Division By Zero", regs);
}
