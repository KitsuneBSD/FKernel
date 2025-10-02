#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>

void nmi_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  kexception("Non Maskable Interrupt",
             "Non-Maskable Interrupt triggered (vector %u)", vector);

  kexception("Non Maskable Interrupt", "RIP=%p CS=%p RFLAGS=%p", frame->rip,
             frame->cs, frame->rflags);
  kexception("Non Maskable Interrupt", "RSP=%p SS=%p", frame->rsp, frame->ss);

  for (;;) {
    asm volatile("hlt");
  }
}
