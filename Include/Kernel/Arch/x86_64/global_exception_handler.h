#pragma once

#include <LibC/stdarg.h>
#include <LibC/stdint.h>

struct CPUStateFrame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

void dump_cpu_state(const CPUStateFrame *frame);
