#include <LibC/stdio.h>

#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>

extern "C" {

void print_stack_trace() {
    struct StackFrame {
        StackFrame* next;
        uint64_t rip;
    };

    StackFrame* frame;
    asm volatile("mov %%rbp, %0" : "=r"(frame));

    fk::algorithms::kexception("PANIC", "--- STACK TRACE ---");
    
    int depth = 0;
    while (frame && depth < 20) {
        if (((uintptr_t)frame & 7) != 0) break;
        
        fk::algorithms::kexception("PANIC", "[%d] %p", depth, (void*)frame->rip);
        frame = frame->next;
        depth++;
    }
    
    if (depth == 0) {
        fk::algorithms::kexception("PANIC", "(No stack trace available - frame pointers might be disabled)");
    }
}

void panic_exception(uint8_t vector, InterruptFrame* frame, const char* message) {
    uint32_t cpu_id = APIC::the().get_id();

    fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    fk::algorithms::kexception("PANIC", "!!!         KERNEL EXCEPTION             !!!");
    fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    fk::algorithms::kexception("PANIC", "CPU ID: %u", cpu_id);
    fk::algorithms::kexception("PANIC", "Exception: %s (vector=%u)", message, (unsigned)vector);
    fk::algorithms::kexception("PANIC", "RIP=%p RSP=%p RFLAGS=%p CS=0x%lx error=0x%lx",
                               (void*)frame->rip, (void*)frame->rsp,
                               (void*)frame->rflags, (unsigned long)frame->cs,
                               (unsigned long)frame->error_code);
    fk::algorithms::kexception("PANIC", "rax=%p rbx=%p rcx=%p rdx=%p",
                               (void*)frame->rax, (void*)frame->rbx,
                               (void*)frame->rcx, (void*)frame->rdx);
    fk::algorithms::kexception("PANIC", "rsi=%p rdi=%p rbp=%p r8=%p",
                               (void*)frame->rsi, (void*)frame->rdi,
                               (void*)frame->rbp, (void*)frame->r8);
    fk::algorithms::kexception("PANIC", "r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p",
                               (void*)frame->r9, (void*)frame->r10, (void*)frame->r11,
                               (void*)frame->r12, (void*)frame->r13,
                               (void*)frame->r14, (void*)frame->r15);

    print_stack_trace();

    fk::algorithms::kfatal("PANIC", "System Halted.");
}

void __kernel_assert_fail(const char *expr, const char *file, int line,
                          const char *func) {
  uint32_t cpu_id = APIC::the().get_id();
  
  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "!!!          KERNEL PANIC                !!!");
  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "CPU ID: %u", cpu_id);
  fk::algorithms::kexception("PANIC", "Reason: Assertion Failed: %s", expr);
  fk::algorithms::kexception("PANIC", "Source: %s:%d", file, line);
  fk::algorithms::kexception("PANIC", "Function: %s", func);
  
  print_stack_trace();
  
  fk::algorithms::kfatal("PANIC", "System Halted.");
}

void __kernel_assert_fail_fmt(const char *expr, const char *file, int line,
                              const char *func, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  uint32_t cpu_id = APIC::the().get_id();

  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "!!!          KERNEL PANIC                !!!");
  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "CPU ID: %u", cpu_id);
  fk::algorithms::kexception("PANIC", "Reason: Assertion Failed: %s", expr);
  fk::algorithms::kexception("PANIC", "Source: %s:%d (%s)", file, line, func);
  fk::algorithms::kexception("PANIC", "Context: %s", buf);

  print_stack_trace();

  fk::algorithms::kfatal("PANIC", "System Halted.");
}

void panic(const char *reason) {
  uint32_t cpu_id = APIC::the().get_id();
  
  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "!!!          KERNEL PANIC                !!!");
  fk::algorithms::kexception("PANIC", "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  fk::algorithms::kexception("PANIC", "CPU ID: %u", cpu_id);
  fk::algorithms::kexception("PANIC", "Reason: %s", reason);
  
  print_stack_trace();
  
  fk::algorithms::kfatal("PANIC", "System Halted.");
}

} // extern "C"
