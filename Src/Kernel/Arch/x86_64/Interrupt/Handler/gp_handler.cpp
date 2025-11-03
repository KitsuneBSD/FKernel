#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

void general_protection_handler([[maybe_unused]] uint8_t vector,
                                InterruptFrame *frame) {
  kexception("General Protection", "General protection interrupt: vector=%u\n",
             (unsigned)vector);
  kexception("General Protection", "RIP=%p CS=%p RFLAGS=%p", frame->rip,
             frame->cs, frame->rflags);
  kexception("General Protection", "RSP=%p SS=%p", frame->rsp, frame->ss);

  for (;;) {
    asm volatile("cli;hlt");
  }
}
