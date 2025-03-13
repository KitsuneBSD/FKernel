#include "../../../../Include/Kernel/Arch/Descriptor/exception_handler.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

void generic_handler() {
  print_str("Exception Catched\n");
  while (1) {
    asm("cli; hlt");
  }
}
