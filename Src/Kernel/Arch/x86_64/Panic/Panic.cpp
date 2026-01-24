#include <LibC/stdio.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

void print_stack_trace() {
    struct StackFrame {
        StackFrame* next;
        uint64_t rip;
    };

    StackFrame* frame;
    asm volatile("mov %%rbp, %0" : "=r"(frame));

    kprintf("\n\033[33m--- STACK TRACE ---\033[0m\n");
    
    int depth = 0;
    while (frame && depth < 20) {
        // Basic sanity check: frame must be aligned and non-null
        if (((uintptr_t)frame & 7) != 0) break;
        
        kprintf("[%d] %p\n", depth, (void*)frame->rip);
        frame = frame->next;
        depth++;
    }
    
    if (depth == 0) {
        kprintf("(No stack trace available - frame pointers might be disabled)\n");
    }
}

void __kernel_assert_fail(const char *expr, const char *file, int line,
                          const char *func) {
  uint32_t cpu_id = APIC::the().get_id();
  
  kprintf("\n\033[31m!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  kprintf("!!!                          KERNEL PANIC                                    !!!\n");
  kprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\033[0m\n\n");
  
  kprintf("CPU ID: %u\n", cpu_id);
  kprintf("Reason: Assertion Failed: %s\n", expr);
  kprintf("Source: %s:%d\n", file, line);
  kprintf("Function: %s\n", func);
  
  print_stack_trace();
  
  kprintf("\n\033[31mSystem Halted.\033[0m\n");

  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

void panic(const char *reason) {
  uint32_t cpu_id = APIC::the().get_id();
  
  kprintf("\n\033[31m!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  kprintf("!!!                          KERNEL PANIC                                    !!!\n");
  kprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\033[0m\n\n");
  
  kprintf("CPU ID: %u\n", cpu_id);
  kprintf("Reason: %s\n", reason);
  
  print_stack_trace();
  
  kprintf("\n\033[31mSystem Halted.\033[0m\n");

  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

} // extern "C"
