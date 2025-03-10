#pragma once

#include "../../../Include/LibK/stdint.h"

#define IDT_MAX_ENTRIES 256
#define GDT_OFFSET_KERNEL_CODE 0x08

extern void *isr_stub_table[];

typedef struct {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t ist;
  uint8_t attributes;
  uint16_t isr_mid;
  uint32_t isr_high;
  uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) idtr_t;

static idt_entry_t idt[IDT_MAX_ENTRIES];
static idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void init_idt();
