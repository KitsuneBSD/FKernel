#pragma once

#include <LibFK/Types/types.h>

/**
 * @file gdt_structures.h
 * @brief Definitions for x86_64 GDT entries, TSS, and related descriptors.
 *
 * Fully compliant with Intel SDM Vol. 3A §3.4 and §7.7.
 * Used by both GDTController and low-level ASM routines.
 */

/**
 * @brief Standard 8-byte Global Descriptor Table (GDT) entry.
 */
struct [[gnu::packed]] GDTEntry {
  uint16_t limit_low;  ///< Lower 16 bits of segment limit
  uint16_t base_low;   ///< Lower 16 bits of base address
  uint8_t base_middle; ///< Bits 16-23 of base address
  uint8_t access;      ///< Access flags (present, ring, type, etc.)
  uint8_t granularity; ///< Granularity (G), operand size (D/B), AVL, etc.
  uint8_t base_high;   ///< Bits 24-31 of base address
};

/**
 * @brief 16-byte Task State Segment (TSS) descriptor for GDT.
 */
struct [[gnu::packed]] TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper; ///< Bits 32–63 of TSS base
  uint32_t reserved;   ///< Must be zero
};

/**
 * @brief GDT Register (GDTR) structure for use with `lgdt`.
 */
struct [[gnu::packed]] GDTR {
  uint16_t limit; ///< Size of GDT in bytes minus one
  uint64_t base;  ///< Linear address of first GDT entry
};

/**
 * @brief 64-bit Task State Segment (TSS) structure.
 *
 * Used for stack switching between privilege levels and for the
 * Interrupt Stack Table (IST) mechanism.
 */
struct [[gnu::packed, gnu::aligned(16)]] TSS64 {
  uint32_t reserved0 = 0;
  uint64_t rsp0 = 0; ///< Ring 0 stack pointer
  uint64_t rsp1 = 0; ///< Ring 1 stack pointer
  uint64_t rsp2 = 0; ///< Ring 2 stack pointer
  uint64_t reserved1 = 0;
  uint64_t ist1 = 0; ///< Interrupt Stack Table entries
  uint64_t ist2 = 0;
  uint64_t ist3 = 0;
  uint64_t ist4 = 0;
  uint64_t ist5 = 0;
  uint64_t ist6 = 0;
  uint64_t ist7 = 0;
  uint64_t reserved2 = 0;
  uint16_t reserved3 = 0;
  uint16_t io_map_base = 0; ///< I/O permission bitmap offset
};

enum SegmentAccess : uint8_t {
  Ring0Code = 0x9A, ///< Present + ring0 + executable + readable
  Ring0Data = 0x92, ///< Present + ring0 + writable
  Ring3Code = 0xFA, ///< Present + ring3 + executable + readable
  Ring3Data = 0xF2, ///< Present + ring3 + writable
  TSS64Type = 0x89  ///< Present + 64-bit TSS (non-busy)
};

enum SegmentFlags : uint8_t {
  Granularity4K = 1 << 7, ///< 4K-byte granularity
  LongMode = 1 << 5       ///< 64-bit code segment
};

constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b) {
  return static_cast<SegmentFlags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

constexpr SegmentAccess operator|(SegmentAccess a, SegmentAccess b) {
  return static_cast<SegmentAccess>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}

/**
 * @brief Helper to create a 64-bit GDT segment descriptor.
 *
 * @param access Access flags (code/data type)
 * @param flags Segment flags (granularity, long mode)
 * @return Encoded 64-bit descriptor value.
 */
static constexpr uint64_t createSegment(SegmentAccess access,
                                        SegmentFlags flags) {
  uint64_t entry = 0;
  entry |= 0xFFFFULL; // Limit (ignored in 64-bit mode)
  entry |= static_cast<uint64_t>(access) << 40; // Access byte
  entry |= static_cast<uint64_t>(flags) << 48;  // Flags
  return entry;
}
