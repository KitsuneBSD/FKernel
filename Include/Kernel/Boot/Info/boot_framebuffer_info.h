#pragma once

#include <LibFK/Types/types.h>

namespace boot {

struct FramebufferInfo {
  uint64_t addr{0};
  uint32_t pitch{0};
  uint32_t width{0};
  uint32_t height{0};
  uint8_t bpp{0};
  uint8_t red_pos{0};
  uint8_t red_mask{0};
  uint8_t green_pos{0};
  uint8_t green_mask{0};
  uint8_t blue_pos{0};
  uint8_t blue_mask{0};
};

} // namespace boot
