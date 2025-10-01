#pragma once

#include <LibC/stdint.h>

struct InterruptFrame {
  // Registradores de uso geral (salvos manualmente)
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;

  // Código de erro (apenas se a interrupção empurrar error_code)
  uint64_t error_code; // 0 se a interrupção não gerar código

  // Registradores empilhados pelo hardware
  uint64_t rip;    // Instruction pointer
  uint64_t cs;     // Code segment
  uint64_t rflags; // Flags
  uint64_t rsp;    // Stack pointer (apenas se troca de privilégio)
  uint64_t ss;     // Stack segment (apenas se troca de privilégio)
};
