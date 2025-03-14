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

  while (1) {
    asm("cli; hlt");
  }
}

void generic_handler(register_t *regs) {
  print_str("[Exception]: Generic Handler\n");
  log_registers(regs);

  panic("Unhandle exception", regs);
}

void invalid_opcode_handler(register_t *regs) {
  print_str("[Exception] Invalid Opcode\n");
  log_registers(regs);

  uint8_t *opcode = (uint8_t *)regs->rip;

  if (*opcode == 0xFF) {
    print_str("Complex opcode detected, cannot safely skip.\n");
    panic("Unhandled invalid opcode", regs);
  }

  regs->rip += 1;
}

void division_by_zero_handler(register_t *regs) {
  print_str("[Exception] Division By Zero\n");
  log_registers(regs);

  panic("Division By Zero", regs);
}
