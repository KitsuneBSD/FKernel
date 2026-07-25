#pragma once

#include <LibFK/Types/types.h>

#ifdef __x86_64__
struct CpuContext {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;

  uint64_t padding;

  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbp;
  uint64_t rbx;

  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rax;
};
#endif

CpuContext GetContextForNewTask(uint64_t stack_pointer, bool is_kernel_task,
                                uint64_t arg1 = 0, uint64_t arg2 = 0);
