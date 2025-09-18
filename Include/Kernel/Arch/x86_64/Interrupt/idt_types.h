#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

struct InterruptFrame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;

  uint64_t error_code;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

struct Idt_Entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

static_assert(sizeof(Idt_Entry) == 16, "IDTDescriptor must be 16 bytes");

struct Idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

using isr_handler_t = void (*)(InterruptFrame *frame, uint8_t vector);
constexpr size_t MAX_X86_64_IDT_SIZE = 256;
