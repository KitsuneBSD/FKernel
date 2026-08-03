#pragma once

#include <LibFK/Types/types.h>

struct IoApicController {
  uintptr_t base = 0;
  uint32_t gsi_base = 0;
  uint32_t max_entries = 0;

  uint32_t read(uint32_t reg) const;
  void write(uint32_t reg, uint32_t value) const;
};
