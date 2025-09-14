#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

constexpr size_t MAX_X86_64_IDT_SIZE = 256;

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
