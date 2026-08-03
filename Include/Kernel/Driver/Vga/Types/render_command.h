#pragma once

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/render_command_type.h>
#include <LibFK/Types/types.h>

struct RenderCommand {
  RenderCommandType type;

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
