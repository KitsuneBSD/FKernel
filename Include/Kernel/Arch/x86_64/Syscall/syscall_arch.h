#pragma once

#include <LibFK/Types/types.h>

constexpr uint32_t MSR_EFER = 0xC0000080;
constexpr uint32_t MSR_STAR = 0xC0000081;
constexpr uint32_t MSR_LSTAR = 0xC0000082;
constexpr uint32_t MSR_SFMASK = 0xC0000084;
constexpr uint32_t MSR_FS_BASE = 0xC0000100;
constexpr uint32_t MSR_GS_BASE = 0xC0000101;
constexpr uint32_t MSR_KERNEL_GS_BASE = 0xC0000102;

constexpr uint64_t EFER_SCE = 1 << 0;
constexpr uint64_t EFER_NXE = 1 << 11;

struct PtRegs {
  uint64_t r9;
  uint64_t r8;
  uint64_t r10;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rax;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbp;
  uint64_t rbx;
  // State pushed for context restoration
  uint64_t rip;
  uint64_t rflags;
  uint64_t rsp;
};

void init_syscalls();
