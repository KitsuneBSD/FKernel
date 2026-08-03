#include <LibC/stdio.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <LibFK/Algorithms/Logging/log.h>

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
