#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

void gp_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  kerror("GP", "General protection interrupt: vector=%u\n", (unsigned)vector);
  kerror("GP", "RIP=%p CS=%p RFLAGS=%p", frame->rip, frame->cs, frame->rflags);
  kerror("GP", "RSP=%p SS=%p", frame->rsp, frame->ss);

  for (;;) {
    asm volatile("cli;hlt");
  }
}
