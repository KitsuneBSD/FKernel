#pragma once

#include "../../../Include/LibK/stdint.h"

#define SEGMENT_DESCRIPTOR_TYPE(x) ((x) << 0x04)
#define SEGMENT_PRESENCE(x) ((x) << 0x07)
#define SEGMENT_AVALIABLE(x) ((x) << 0x0C)
#define SEGMENT_LONG_MODE(x) ((x) << 0x0D)
#define SEGMENT_SIZE(x) ((x) << 0x0E)
#define SEGMENT_GRANULARITY(x) ((x) << 0x0F)
#define SEGMENT_PRIVILEGE(x) (((x) & 0x03) << 0x05)

#define SEGMENT_DATA_READ_ONLY 0x00
#define SEGMENT_DATA_READ_ONLY_ACCESSED 0x01

#define SEGMENT_DATA_READ_WRITE 0x02
#define SEGMENT_DATA_READ_WRITE_ACCESSED 0x03

#define SEGMENT_DATA_READ_ONLY_EXPAND_DOWN 0x04
#define SEGMENT_DATA_READ_ONLY_EXPAND_DOWN_ACCESSED 0x05

#define SEGMENT_DATA_READ_WRITE_EXPAND_DOWN 0x06
#define SEGMENT_DATA_READ_WRITE_EXPAND_DOWN_ACCESSED 0x07

#define SEGMENT_CODE_EXECUTE_ONLY 0x08
#define SEGMENT_CODE_EXECUTE_ONLY_ACCESSED 0x09

#define SEGMENT_CODE_EXECUTE_READ 0x0a
#define SEGMENT_CODE_EXECUTE_READ_ACCESSED 0x0b

#define SEGMENT_CODE_EXECUTE_ONLY_CONFORMING 0x0c
#define SEGMENT_CODE_EXECUTE_ONLY_CONFORMING_ACCESSED 0x0d

#define SEGMENT_CODE_EXECUTE_READ_CONFORMING 0x0e
#define SEGMENT_CODE_EXECUTE_READ_CONFORMING_ACCESSED 0x0f

#define GDT_CODE_RING_0                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(0) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_0                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(0) | SEGMENT_DATA_READ_WRITE)

#define GDT_CODE_RING_1                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(1) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_1                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(1) | SEGMENT_DATA_READ_WRITE)

#define GDT_CODE_RING_2                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(2) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_2                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(2) | SEGMENT_DATA_READ_WRITE)

#define GDT_CODE_RING_3                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(3) | SEGMENT_CODE_EXECUTE_READ)

#define GDT_DATA_RING_3                                                        \
  (SEGMENT_DESCRIPTOR_TYPE(1) | SEGMENT_PRESENCE(1) | SEGMENT_AVALIABLE(0) |   \
   SEGMENT_LONG_MODE(0) | SEGMENT_SIZE(1) | SEGMENT_GRANULARITY(1) |           \
   SEGMENT_PRIVILEGE(3) | SEGMENT_DATA_READ_WRITE)

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

void init_gdt();
void create_descriptor(uint32_t base, uint32_t limit, uint16_t flag);
