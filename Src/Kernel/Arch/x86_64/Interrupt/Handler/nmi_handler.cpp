#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>

void nmi_handler([[maybe_unused]] uint8_t vector, InterruptFrame *frame) {
  kexception("Non Maskable Interrupt",
             "Non-Maskable Interrupt triggered (vector %u)", vector);

  kexception("Non Maskable Interrupt", "RIP=%p CS=%p RFLAGS=%p", frame->rip,
             frame->cs, frame->rflags);
  kexception("Non Maskable Interrupt", "RSP=%p SS=%p", frame->rsp, frame->ss);
  kexception("Non Maskable Interrupt", "RAX=%p RBX=%p RCX=%p RDX=%p",
             frame->rax, frame->rbx, frame->rcx, frame->rdx);
  kexception("Non Maskable Interrupt", "RSI=%p RDI=%p RBP=%p", frame->rsi,
             frame->rdi, frame->rbp);
  kexception("Non Maskable Interrupt", "R8=%p R9=%p R10=%p R11=%p", frame->r8,
             frame->r9, frame->r10, frame->r11);
  kexception("Non Maskable Interrupt", "R12=%p R13=%p R14=%p R15=%p",
             frame->r12, frame->r13, frame->r14, frame->r15);

  for (;;) {
    asm volatile("hlt");
  }
}
