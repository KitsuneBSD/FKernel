#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

void default_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  kerror("EXCEPTION", "Unhandled interrupt: vector=%u\n", (unsigned)vector);
  kerror("EXCEPTION", "RIP=%p CS=%p RFLAGS=%p", frame->rip, frame->cs,
         frame->rflags);
  kerror("EXCEPTION", "RSP=%p SS=%p", frame->rsp, frame->ss);

  for (;;) {
    asm volatile("hlt");
  }
}
