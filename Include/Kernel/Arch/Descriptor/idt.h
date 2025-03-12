#pragma once

#include "../../../../Include/Kernel/LibK/stdint.h"

struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

extern struct idt_entry idt[IDT_ENTRIES];
extern struct idt_ptr idtp;

void init_idt();
extern void idt_flush(uint64_t idtp_address);
