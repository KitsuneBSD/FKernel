#pragma once

#include <Arch/x86_64/stack_size.h>
#include <LibC/stdint.h>

struct __attribute__((packed)) GDTEntry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
};

static_assert(sizeof(GDTEntry) == 8, "GDT_ENTRY must be 8 bytes");

struct __attribute__((packed)) GDT_TSS_Entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper;
  uint32_t reserved;
};

static_assert(sizeof(GDT_TSS_Entry) == 16, "GDT_TSS_Entry must be 16 bytes");

struct __attribute__((packed)) GDTPointer {
  uint16_t limit;
  uint64_t base;
};

static_assert(sizeof(GDTPointer) == 10, "GDTPointer must be 10 bytes");

struct __attribute__((packed)) TSS {
  uint32_t reserved0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base;
};

static_assert(sizeof(TSS) == 104, "TSS must be 104 bytes (64-bit TSS)");

extern "C" void gdt_flush(struct GDTPointer *gdtr);
extern "C" void tss_flush(uint16_t selector);

void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access,
               uint8_t gran);
void set_tss_descriptor(uint64_t base, uint32_t limit);

void init_gdt();
