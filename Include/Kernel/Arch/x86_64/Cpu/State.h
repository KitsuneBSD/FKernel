#pragma once

#include <LibC/stdint.h>

struct [[gnu::packed]] CpuState {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t interrupt_id;
  uint64_t error_code;
  uint64_t rip, cs, rflags, rsp, ss;
};
