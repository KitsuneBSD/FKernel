#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibC/stdint.h>
#include <LibC/stdio.h>

void default_handler([[maybe_unused]] uint8_t vector) {
  kprintf("Unhandled interrupt: vector=%u\n", (unsigned)vector);
  for (;;) {
    asm volatile("hlt");
  }
}
