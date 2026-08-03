#pragma once

#include <LibFK/Types/types.h>

namespace VgaExtended {

struct Glyph {
  uint8_t width;
  uint8_t height;
  uint8_t data[16][8];
  uint8_t utf8_bytes;
};

} // namespace VgaExtended
