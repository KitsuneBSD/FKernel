#include "../../../../Include/Kernel/Arch/Descriptor/exception_handler.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

void generic_handler(register_t *regs) {
  print_str("Exception Catched\n");
  print_int((int)regs->int_no);
  print_str("\n");
  while (1) {
    asm("cli; hlt");
  }
}

void division_by_zero_handler(register_t *regs) {
  print_str("Division by Zero!\n");

  if (regs->rip) {
    regs->rax = 0;
    regs->rip += 2;
    return;
  }

  print_str("Can't fix the stack change the result to 0\n");
}
