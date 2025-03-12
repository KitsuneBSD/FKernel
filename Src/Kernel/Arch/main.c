#include "../../../Include/Kernel/Arch/Descriptor/gdt.h"
#include "../../../Include/Kernel/Arch/Descriptor/idt.h"
#include "../../../Include/Kernel/Driver/vga_buffer.h"

void kmain() {
  clear_screen();
  init_gdt();
  init_idt();
  print_str("[OK]: There is the latest point in the kernel");
}
