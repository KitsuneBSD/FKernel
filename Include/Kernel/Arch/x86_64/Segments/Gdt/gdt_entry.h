#pragma once 

#include <LibFK/Types/types.h>

/**
 * @brief Standard 8-byte Global Descriptor Table (GDT) entry.
 */
struct GDTEntry {
  uint16_t limit_low;  ///< Lower 16 bits of segment limit
  uint16_t base_low;   ///< Lower 16 bits of base address
  uint8_t base_middle; ///< Bits 16-23 of base address
  uint8_t access;      ///< Access flags (present, ring, type, etc.)
  uint8_t granularity; ///< Granularity (G), operand size (D/B), AVL, etc.
  uint8_t base_high;   ///< Bits 24-31 of base address
} __attribute__((packed));

static_assert(sizeof(GDTEntry) == 8, "GDTEntry must be 8 bytes in size");