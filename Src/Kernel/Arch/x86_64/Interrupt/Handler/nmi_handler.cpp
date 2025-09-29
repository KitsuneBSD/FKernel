#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/log.h>

void nmi_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  kerror("NMI", "Non-Maskable Interrupt triggered (vector %u)", vector);

  kerror("NMI", "RIP=%p CS=%p RFLAGS=%p", frame->rip, frame->cs, frame->rflags);
  kerror("NMI", "RSP=%p SS=%p", frame->rsp, frame->ss);

  for (;;) {
    asm volatile("hlt");
  }
}
