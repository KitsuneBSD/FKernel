#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Represents the CPU state saved during an interrupt.
 *
 * This struct holds the values of general-purpose registers,
 * segment registers, instruction pointer, flags, and stack pointer
 * at the moment an interrupt occurs. It is typically used in
 * interrupt handlers to inspect or modify the CPU state.
 */
struct InterruptFrame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t error_code;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};