#include <Kernel/Arch/x86_64/Interrupt/handlerList.h>

#include <LibC/stdio.h>

void default_handler([[maybe_unused]] InterruptFrame *frame,
                     [[maybe_unused]] uint8_t vector) {
  kprintf("Unhandled interrupt: vector=%u\n", (unsigned)vector);
  for (;;) {
    asm volatile("hlt");
  }
}
