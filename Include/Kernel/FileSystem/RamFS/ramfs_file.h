#pragma once

#include <LibFK/Types/types.h>

struct RamFile {
  char r_name[256];
  uint8_t r_data[1024];
  size_t r_size{0};
};
