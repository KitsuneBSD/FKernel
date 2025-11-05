#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

void general_protection_handler([[maybe_unused]] uint8_t vector,
                                InterruptFrame *frame) {
  kexception("General Protection", "General protection interrupt: vector=%u",
             (unsigned)vector);
  kexception("General Protection", "RIP=%p CS=%p RFLAGS=%p", frame->rip,
             frame->cs, frame->rflags);
  kexception("General Protection", "RSP=%p SS=%p", frame->rsp, frame->ss);
  kexception("General Protection", "RAX=%p RBX=%p RCX=%p RDX=%p", frame->rax,
             frame->rbx, frame->rcx, frame->rdx);
  kexception("General Protection", "RSI=%p RDI=%p RBP=%p", frame->rsi,
             frame->rdi, frame->rbp);
  kexception("General Protection", "R8=%p R9=%p R10=%p R11=%p", frame->r8,
             frame->r9, frame->r10, frame->r11);
  kexception("General Protection", "R12=%p R13=%p R14=%p R15=%p", frame->r12,
             frame->r13, frame->r14, frame->r15);

  for (;;) {
    asm volatile("cli;hlt");
  }
}
