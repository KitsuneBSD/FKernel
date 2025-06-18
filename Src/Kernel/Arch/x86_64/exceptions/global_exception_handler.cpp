#include <Arch/x86_64/global_exception_handler.h>
#include <Kernel/Driver/Vga_Buffer.hpp>
#include <LibFK/Log.h>

extern "C" void halt() {
  asm volatile("cli; hlt");
  while (true) {
    asm volatile("hlt");
  }
}

void register_exception_handler(uint8_t vector, ExceptionHandler handler) {
  if (vector < 0 || vector >= IDT_ENTRIES) {
    Logf(LogLevel::ERROR, "IDT vector out of bounds: %d", vector);
  }

  if (vector < 32) {
    exception_handlers[vector] = handler;
  }
}

void handle_double_fault(const CPUStateFrame *frame) {
  Logf(LogLevel::ERROR,
       "[CRITICAL] Double Fault occurred! System state likely corrupted.");
  Logf(LogLevel::ERROR, "This usually indicates a stack overflow, invalid IDT, "
                        "or another fault inside a fault.");

  Logf(LogLevel::ERROR, "RIP=%016lx, RSP=%016lx, RFLAGS=%016lx", frame->rip,
       frame->rsp, frame->rflags);
  Logf(LogLevel::ERROR, "System halted.");

  while (true)
    asm volatile("cli; hlt");
}

void default_exception_handler(const CPUStateFrame *frame) {
  if (frame->int_no < 32)
    Logf(LogLevel::WARN, "Interrupt %lu: %s", frame->int_no,
         interrupt_messages[frame->int_no]);
  else
    Logf(LogLevel::WARN, "Interrupt %lu: Unknown or external IRQ",
         frame->int_no);

  dump_registers(frame);
  dump_stack(frame);

  Logf(LogLevel::WARN, "=====================");

  while (true)
    asm volatile("cli; hlt");
}

void named_interrupt(uint64_t int_no) {
  if (int_no < 32) {
    Logf(LogLevel::WARN, "Interrupt %lu: %s", int_no,
         interrupt_messages[int_no]);
  } else {
    Logf(LogLevel::WARN, "Interrupt %lu: Unknown or external IRQ", int_no);
  }
}

extern "C" void
general_protection_fault_handler(const struct CPUStateFrame *frame) {
  Logf(LogLevel::ERROR, "!!! GENERAL PROTECTION FAULT !!!");
  Logf(LogLevel::ERROR, "RIP: 0x%016lx CS: 0x%016lx FLAGS: 0x%016lx",
       frame->rip, frame->cs, frame->rflags);
  Logf(LogLevel::ERROR, "RSP: 0x%016lx SS: 0x%016lx", frame->rsp, frame->ss);
  Logf(LogLevel::ERROR, "Error Code: 0x%016lx", frame->err_code);
  Logf(LogLevel::ERROR, "System halted to prevent damage.");

  halt();
}

void dump_cpu_state(const CPUStateFrame *frame) {
  if (!frame) {
    Logf(LogLevel::ERROR, "[PANIC] CPU Frame NULL");
    return;
  }

  Logf(LogLevel::WARN, "=== CPU STATE DUMP ===");

  if (frame->int_no < 32 && exception_handlers[frame->int_no]) {
    exception_handlers[frame->int_no](frame);
    return;
  }

  named_interrupt(frame->int_no);
  dump_registers(frame);
  dump_stack(frame);

  Logf(LogLevel::WARN, "=======================");
}

void init_exception_handlers() {
  for (int i = 0; i < 32; ++i)
    exception_handlers[i] = default_exception_handler;

  register_exception_handler(13, general_protection_fault_handler);
  register_exception_handler(8, handle_double_fault);

  Log(LogLevel::INFO, "Exception Handlers initialized with sucess");
}
