#include "../../../../Include/Kernel/Arch/Descriptor/exception_handler.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

void generic_handler() {
  print_str("Generic Handler exception catch\n");
  while (1) {
    asm("cli; hlt");
  }
}

void invalid_opcode_handler(register_t *regs) {
  print_str("Invalid Opcode Handler exception catch\n");

  uint8_t *faulting_instruction = (uint8_t *)regs->rip;

  if (*faulting_instruction == 0x0F) {
    regs->rip += 2;
  } else {
    regs->rip += 1;
  }
}

void division_by_zero_handler(register_t *regs) {
  print_str("Division By Zero Handler exception catch\n");
  if (regs->rip) {
    regs->rax = 0;
    regs->rip += 2;
    return;
  }

  print_str("Can't jump the operation");
  while (1) {
    asm("cli; hlt");
  }
}
