#pragma once

/**
 * @file gdt_structures.h
 * @brief Definitions for x86_64 GDT entries, TSS, and related descriptors.
 *
 * Fully compliant with Intel SDM Vol. 3A §3.4 and §7.7.
 * Used by both GDTController and low-level ASM routines.
 */

#include <Kernel/Arch/x86_64/Segments/Gdt/gdt_entry.h>
#include <Kernel/Arch/x86_64/Segments/Gdt/gdt_pointer.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_descriptor.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_structures.h>

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

  entry |= 0xFFFFULL;              // limite
  entry |= (uint64_t)access << 40; // access byte
  entry |= (uint64_t)flags << 48;  // flags+granularity

  return entry;
}
