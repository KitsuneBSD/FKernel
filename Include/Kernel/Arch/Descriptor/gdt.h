#pragma once

#include "../../../../Include/Kernel/LibK/stdint.h"

#define SEGMENT_DESCRIPTOR_TYPE(x) ((x) << 4)
#define SEGMENT_PRESENCE(x) ((x) << 7)
#define SEGMENT_AVAILABLE(x) ((x) << 12)
#define SEGMENT_LONG_MODE(x) ((x) << 13)
#define SEGMENT_SIZE(x) ((x) << 14)
#define SEGMENT_GRANULARITY(x) ((x) << 15)
#define SEGMENT_PRIVILEGE(x) (((x) & 0x03) << 5)

#define SEGMENT_DATA_READ_WRITE 0x02
#define SEGMENT_CODE_EXECUTE 0x0A

#define GDT_CODE_RING_0                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVAILABLE(0) |   \
   SEGMENT_LONG_MODE(1) | SEGMENT_SIZE(0) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(0) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_0                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVAILABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(0) | SEGMENT_DATA_READ_WRITE)

#define GDT_CODE_RING_3                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVAILABLE(0) |   \
   SEGMENT_LONG_MODE(1) | SEGMENT_SIZE(0) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(3) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_3                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVAILABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(3) | SEGMENT_DATA_READ_WRITE)

struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

#define MAX_GDT_ENTRIES 5

extern struct gdt_entry gdt[MAX_GDT_ENTRIES];
extern struct gdt_ptr gdtr;

void init_gdt();
extern void gdt_flush(uint64_t gdtp_address);
