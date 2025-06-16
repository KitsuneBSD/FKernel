#pragma once

#include <Arch/x86_64/stack_size.h>
#include <LibC/stdarg.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

using StackWord = uintptr_t;
using ExceptionHandler = void (*)(const struct CPUStateFrame *);

constexpr const char *interrupt_messages[32] = {
    "Divide-by-zero Error",          // 0
    "Debug Exception",               // 1
    "Non-maskable Interrupt (NMI)",  // 2
    "Breakpoint",                    // 3
    "Overflow",                      // 4
    "Bound Range Exceeded",          // 5
    "Invalid Opcode",                // 6
    "Device Not Available",          // 7
    "Double Fault",                  // 8
    "Coprocessor Segment Overrun",   // 9
    "Invalid TSS",                   // 10
    "Segment Not Present",           // 11
    "Stack-Segment Fault",           // 12
    "General Protection Fault",      // 13
    "Page Fault",                    // 14
    "Reserved",                      // 15
    "x87 FPU Floating-Point Error",  // 16
    "Alignment Check",               // 17
    "Machine Check",                 // 18
    "SIMD Floating-Point Exception", // 19
    "Virtualization Exception",      // 20
    "Reserved",
    "Reserved",
    "Reserved", // 21–23
    "Reserved",
    "Reserved",
    "Reserved", // 24–26
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved" // 27–31
};

static ExceptionHandler exception_handlers[32] = {nullptr};

void register_exception_handler(uint8_t vector, ExceptionHandler handler);

struct CPUStateFrame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

void handle_double_fault(const struct CPUStateFrame *frame);

void init_exception_handlers();

void named_interrupt(uint64_t int_no);

static void dump_stack(const CPUStateFrame *frame, int count = 5) {
  StackWord *stack = reinterpret_cast<StackWord *>(frame->rsp);
  Logf(LogLevel::WARN, "Stack top:");

  for (int i = 0; i < count; ++i) {
    Logf(LogLevel::WARN, " [%p] %016lx", stack + i, stack[i]);
  }
}

static void dump_registers(const CPUStateFrame *frame) {
  Logf(LogLevel::WARN, "RIP=%016lx, RSP=%016lx, RFLAGS=%016lx", frame->rip,
       frame->rsp, frame->rflags);
  Logf(LogLevel::WARN, "CS=%04lx, SS=%04lx, Interrupt=%lu, ErrorCode=%lx",
       frame->cs, frame->ss, frame->int_no, frame->err_code);
}

extern "C" void dump_cpu_state(const CPUStateFrame *frame);
