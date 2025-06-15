#include <Arch/x86_64/global_exception_handler.h>
#include <LibFK/Log.h>

void dump_cpu_state(const CPUStateFrame *frame) {
  if (!frame) {
    Logf(LogLevel::ERROR, "[KERNEL PANIC] CPU frame is null.\n");
    return;
  }

  Logf(LogLevel::WARN, "\n=== CPU State Dump ===\n");

  Logf(LogLevel::WARN,
       "RAX: 0x%016lx  RBX: 0x%016lx  RCX: 0x%016lx  RDX: 0x%016lx\n",
       frame->rax, frame->rbx, frame->rcx, frame->rdx);
  Logf(LogLevel::WARN,
       "RSI: 0x%016lx  RDI: 0x%016lx  RBP: 0x%016lx  RSP: 0x%016lx\n",
       frame->rsi, frame->rdi, frame->rbp, frame->rsp);
  Logf(LogLevel::WARN,
       " R8: 0x%016lx   R9: 0x%016lx  R10: 0x%016lx  R11: 0x%016lx\n",
       frame->r8, frame->r9, frame->r10, frame->r11);
  Logf(LogLevel::WARN,
       "R12: 0x%016lx  R13: 0x%016lx  R14: 0x%016lx  R15: 0x%016lx\n",
       frame->r12, frame->r13, frame->r14, frame->r15);

  Logf(LogLevel::WARN, "\nRIP: 0x%016lx  CS:  0x%04lx\n", frame->rip,
       frame->cs);
  Logf(LogLevel::WARN, "RFLAGS: 0x%016lx\n", frame->rflags);
  Logf(LogLevel::WARN, "SS:  0x%04lx\n", frame->ss);
  Logf(LogLevel::WARN, "INT: 0x%02lx  ERR: 0x%016lx\n", frame->int_no,
       frame->err_code);

  Logf(LogLevel::WARN, "======================\n");

  uintptr_t *stack = (uintptr_t *)frame->rsp;
  Logf(LogLevel::WARN, "\nStack (top 5):\n");
  for (int i = 0; i < 5; ++i) {
    Logf(LogLevel::WARN, "  [%p] 0x%016lx\n", stack + i, stack[i]);
  }
}
