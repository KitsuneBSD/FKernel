#pragma once

#include <Kernel/Driver/Vga/Types/color.h>
#include <LibFK/Types/types.h>

struct RenderCommand {
  enum class Type { PutChar, Scroll, Clear, SetColor, Flush } type;

  union {
    struct {
      uint32_t codepoint;
    } put_char;
    struct {
      Color fg;
      Color bg;
    } set_color;
  } data;
};
