#pragma once

#define MAX_EXCEPTIONS 32

#include "../../../../Include/Kernel/LibK/stdint.h"

typedef struct {
  uint64_t ds;

  uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;

  uint64_t int_no, err_code;

  uint64_t rip, cs, rflags, user_rsp, ss;
} register_t;

void panic(const char *segpoint, register_t *regs);
void generic_handler(register_t *regs);
void division_by_zero_handler(register_t *regs);
void invalid_opcode_handler(register_t *regs);
