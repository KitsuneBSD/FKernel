#pragma once 

#include <LibFK/Types/types.h>

/**
 * @brief GDT Register (GDTR) structure for use with `lgdt`.
 */
struct GDTR {
  uint16_t limit; ///< Size of GDT in bytes minus one
  uint64_t base;  ///< Linear address of first GDT entry
} __attribute__((packed));

static_assert(sizeof(GDTR) == 10, "GDTR must be 10 bytes in size");