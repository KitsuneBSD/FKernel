#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Information about the current framebuffer
 */
struct FramebufferInfo {
  uint8_t *address;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;
};
