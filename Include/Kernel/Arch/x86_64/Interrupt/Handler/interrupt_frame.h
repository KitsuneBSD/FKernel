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
  /// General-purpose register RAX
  uint64_t rax;

  /// General-purpose register RBX
  uint64_t rbx;

  /// General-purpose register RCX
  uint64_t rcx;

  /// General-purpose register RDX
  uint64_t rdx;

  /// Source index register (RSI)
  uint64_t rsi;

  /// Destination index register (RDI)
  uint64_t rdi;

  /// Base pointer register (RBP)
  uint64_t rbp;

  /// General-purpose register R8
  uint64_t r8;

  /// General-purpose register R9
  uint64_t r9;

  /// General-purpose register R10
  uint64_t r10;

  /// General-purpose register R11
  uint64_t r11;

  /// General-purpose register R12
  uint64_t r12;

  /// General-purpose register R13
  uint64_t r13;

  /// General-purpose register R14
  uint64_t r14;

  /// General-purpose register R15
  uint64_t r15;

  /// Error code pushed by the CPU (if applicable)
  uint64_t error_code;

  /// Instruction pointer at the time of the interrupt
  uint64_t rip;

  /// Code segment selector
  uint64_t cs;

  /// CPU flags at the time of the interrupt
  uint64_t rflags;

  /// Stack pointer at the time of the interrupt
  uint64_t rsp;

  /// Stack segment selector
  uint64_t ss;
};
