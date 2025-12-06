#pragma once 

#include <LibFK/Types/types.h>

/**
 * @brief 16-byte Task State Segment (TSS) descriptor for GDT.
 */
struct TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper; ///< Bits 32–63 of TSS base
  uint32_t reserved;   ///< Must be zero
} __attribute__((packed));

static_assert(sizeof(TSSDescriptor) == 16,
              "TSSDescriptor must be 16 bytes in size");