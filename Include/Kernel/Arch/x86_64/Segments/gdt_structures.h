#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Standard Global Descriptor Table (GDT) entry
 */
struct GDTEntry {
  uint16_t limit_low;  ///< Lower 16 bits of the segment limit
  uint16_t base_low;   ///< Lower 16 bits of the base address
  uint8_t base_middle; ///< Middle 8 bits of the base address
  uint8_t access;      ///< Access flags (present, ring, executable, etc.)
  uint8_t granularity; ///< Granularity and segment size flags
  uint8_t base_high;   ///< High 8 bits of the base address
} __attribute__((packed));

/**
 * @brief Task State Segment (TSS) descriptor for GDT
 */
struct TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper; ///< Base bits 32-63
  uint32_t reserved;
} __attribute__((packed));

/**
 * @brief GDT pointer used by `lgdt` instruction
 */
struct GDTR {
  uint16_t limit; ///< Size of GDT in bytes minus 1
  uint64_t base;  ///< Address of the first GDT entry
} __attribute__((packed));

/**
 * @brief 64-bit Task State Segment (TSS) structure
 */
struct [[gnu::packed]] TSS64 {
  uint32_t reserved0;
  uint64_t rsp0; ///< Ring 0 stack pointer
  uint64_t rsp1; ///< Ring 1 stack pointer
  uint64_t rsp2; ///< Ring 2 stack pointer
  uint64_t reserved1;
  uint64_t ist1; ///< Interrupt Stack Table entry 1
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base; ///< I/O permission bitmap offset
};

/**
 * @brief Segment access types
 */
enum SegmentAccess : uint8_t {
  Ring0Code = 0x9A, ///< Present + ring0 + executable + readable
  Ring0Data = 0x92, ///< Present + ring0 + writable
  TSS64 = 0x89      ///< Present + 64-bit TSS
};

/**
 * @brief Segment flags
 */
enum SegmentFlags : uint8_t {
  Granularity4K = 1 << 7, ///< 4K-byte granularity
  LongMode = 1 << 5       ///< 64-bit segment
};

/**
 * @brief Bitwise OR operator for SegmentFlags
 */
constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b) {
  return static_cast<SegmentFlags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

/**
 * @brief Bitwise OR operator for SegmentAccess
 */
constexpr SegmentAccess operator|(SegmentAccess a, SegmentAccess b) {
  return static_cast<SegmentAccess>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}

/**
 * @brief Helper to create a 64-bit GDT segment descriptor
 *
 * @param access Segment access flags
 * @param flags Segment flags (granularity, long mode)
 * @return 64-bit encoded GDT entry
 */
static constexpr uint64_t createSegment(SegmentAccess access,
                                        SegmentFlags flags) {
  uint64_t seg = 0;
  seg |= static_cast<uint64_t>(access) << 40;
  seg |= static_cast<uint64_t>(flags) << 48;
  seg |= 0xFFFFULL; // low 16 bits of limit
  return seg;
}
