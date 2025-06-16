#include <Arch/x86_64/global_exception_handler.h>
#include <Kernel/Driver/Vga_Buffer.hpp>
#include <LibFK/Log.h>

void named_interrupt(uint64_t int_no) {
  switch (int_no) {
  case 0:
    Logf(LogLevel::WARN, "Interrupt 0: Divide-by-zero Error\n");
    break;
  case 1:
    Logf(LogLevel::WARN, "Interrupt 1: Debug Exception\n");
    break;
  case 2:
    Logf(LogLevel::WARN, "Interrupt 2: Non-maskable Interrupt (NMI)\n");
    break;
  case 3:
    Logf(LogLevel::WARN, "Interrupt 3: Breakpoint\n");
    break;
  case 4:
    Logf(LogLevel::WARN, "Interrupt 4: Overflow\n");
    break;
  case 5:
    Logf(LogLevel::WARN, "Interrupt 5: Bound Range Exceeded\n");
    break;
  case 6:
    Logf(LogLevel::WARN, "Interrupt 6: Invalid Opcode\n");
    break;
  case 7:
    Logf(LogLevel::WARN, "Interrupt 7: Device Not Available\n");
    break;
  case 8:
    Logf(LogLevel::WARN, "Interrupt 8: Double Fault\n");
    break;
  case 9:
    Logf(LogLevel::WARN, "Interrupt 9: Coprocessor Segment Overrun\n");
    break;
  case 10:
    Logf(LogLevel::WARN, "Interrupt 10: Invalid TSS\n");
    break;
  case 11:
    Logf(LogLevel::WARN, "Interrupt 11: Segment Not Present\n");
    break;
  case 12:
    Logf(LogLevel::WARN, "Interrupt 12: Stack-Segment Fault\n");
    break;
  case 13:
    Logf(LogLevel::WARN, "Interrupt 13: General Protection Fault\n");
    break;
  case 14:
    Logf(LogLevel::WARN, "Interrupt 14: Page Fault\n");
    break;
  case 15:
    Logf(LogLevel::WARN, "Interrupt 15: Reserved\n");
    break;
  case 16:
    Logf(LogLevel::WARN, "Interrupt 16: x87 FPU Floating-Point Error\n");
    break;
  case 17:
    Logf(LogLevel::WARN, "Interrupt 17: Alignment Check\n");
    break;
  case 18:
    Logf(LogLevel::WARN, "Interrupt 18: Machine Check\n");
    break;
  case 19:
    Logf(LogLevel::WARN, "Interrupt 19: SIMD Floating-Point Exception\n");
    break;
  case 20:
    Logf(LogLevel::WARN, "Interrupt 20: Virtualization Exception\n");
    break;
  case 21:
    Logf(LogLevel::WARN, "Interrupt 21: Reserved\n");
    break;
  case 22:
    Logf(LogLevel::WARN, "Interrupt 22: Reserved\n");
    break;
  case 23:
    Logf(LogLevel::WARN, "Interrupt 23: Reserved\n");
    break;
  case 24:
    Logf(LogLevel::WARN, "Interrupt 24: Reserved\n");
    break;
  case 25:
    Logf(LogLevel::WARN, "Interrupt 25: Reserved\n");
    break;
  case 26:
    Logf(LogLevel::WARN, "Interrupt 26: Reserved\n");
    break;
  case 27:
    Logf(LogLevel::WARN, "Interrupt 27: Reserved\n");
    break;
  case 28:
    Logf(LogLevel::WARN, "Interrupt 28: Reserved\n");
    break;
  case 29:
    Logf(LogLevel::WARN, "Interrupt 29: Reserved\n");
    break;
  case 30:
    Logf(LogLevel::WARN, "Interrupt 30: Reserved\n");
    break;
  case 31:
    Logf(LogLevel::WARN, "Interrupt 31: Reserved\n");
    break;
  default:
    Logf(LogLevel::WARN, "Interrupt %lu: Unknown or external IRQ\n", int_no);
    break;
  }
}

extern "C" void dump_cpu_state(const CPUStateFrame *frame) {
  if (!frame) {
    Logf(LogLevel::ERROR, "[PANIC] CPU Frame NULL\n");
    return;
  }

  Logf(LogLevel::WARN, "=== CPU STATE DUMP ===\n");

  // TODO: USE CPUStateFrame to make a custom treat for each exception

  named_interrupt(frame->int_no);

  Logf(LogLevel::WARN, "Stack top:\n");
  uintptr_t *stack = reinterpret_cast<uintptr_t *>(frame->rsp);
  for (int i = 0; i < 5; ++i)
    Logf(LogLevel::WARN, " [%p] %016lx\n", stack + i, stack[i]);

  Logf(LogLevel::WARN, "=======================\n");
}
