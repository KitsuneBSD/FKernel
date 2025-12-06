#pragma once 

#include <LibFK/Types/types.h>

/**
 * @brief 64-bit Task State Segment (TSS) structure.
 *
 * Used for stack switching between privilege levels and for the
 * Interrupt Stack Table (IST) mechanism.
 */
struct TSS64 {
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
}__attribute__((packed, aligned(16)));

static_assert(sizeof(TSS64) == 112,
              "TSS64 must be 112 bytes in size");