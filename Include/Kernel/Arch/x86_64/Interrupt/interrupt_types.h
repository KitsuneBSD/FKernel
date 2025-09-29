#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <LibC/stddef.h>

constexpr size_t MAX_x86_64_IDT_SIZE = 256;

struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

static_assert(sizeof(idt_entry) == 16, "IDTDescriptor must be 16 bytes");

struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

using interrupt = void (*)(uint8_t vector, InterruptFrame *frame);
