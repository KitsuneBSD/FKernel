#pragma once

#include <Arch/x86_64/stack_size.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

struct IDTEntry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

struct IDTPointer {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

extern "C" void flush_idt(IDTPointer *idtr);

void init_idt();
