#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/log.h>

display_framebuffer::display_framebuffer() : cursor_x(0), cursor_y(0) {
  initialize_framebuffer();
  clear();
}

void display_framebuffer::initialize_framebuffer() {
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    framebuffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(fb.addr));
    fb_width = fb.width;
    fb_height = fb.height;
    fb_pitch = fb.pitch;
    fb_bpp = fb.bpp;

    fk::algorithms::klog("DISPLAY", "Framebuffer: %ux%u %ubpp at %p (pitch %u)", 
                         fb_width, fb_height, fb_bpp, framebuffer, fb_pitch);
    fk::algorithms::klog("DISPLAY", "  Red: pos=%u mask=%u, Green: pos=%u mask=%u, Blue: pos=%u mask=%u",
                         fb.red_pos, fb.red_mask, fb.green_pos, fb.green_mask, fb.blue_pos, fb.blue_mask);
  } else {
    framebuffer = nullptr;
    fk::algorithms::kwarn("DISPLAY", "No framebuffer found in BootInfo!");
  }
}

uint32_t display_framebuffer::color_to_pixel(Color c) const {
  uint32_t r = 0, g = 0, b = 0;

  switch (c) {
  case Color::Black: r = 0x00; g = 0x00; b = 0x00; break;
  case Color::Blue: r = 0x00; g = 0x00; b = 0xAA; break;
  case Color::Green: r = 0x00; g = 0xAA; b = 0x00; break;
  case Color::Cyan: r = 0x00; g = 0xAA; b = 0xAA; break;
  case Color::Red: r = 0xAA; g = 0x00; b = 0x00; break;
  case Color::Magenta: r = 0xAA; g = 0x00; b = 0xAA; break;
  case Color::Brown: r = 0xAA; g = 0x55; b = 0x00; break;
  case Color::LightGray: r = 0xAA; g = 0xAA; b = 0xAA; break;
  case Color::DarkGray: r = 0x55; g = 0x55; b = 0x55; break;
  case Color::LightBlue: r = 0x55; g = 0x55; b = 0xFF; break;
  case Color::LightGreen: r = 0x55; g = 0xFF; b = 0x55; break;
  case Color::LightCyan: r = 0x55; g = 0xFF; b = 0xFF; break;
  case Color::LightRed: r = 0xFF; g = 0x55; b = 0x55; break;
  case Color::LightMagenta: r = 0xFF; g = 0x55; b = 0xFF; break;
  case Color::Yellow: r = 0xFF; g = 0xFF; b = 0x55; break;
  case Color::White: r = 0xFF; g = 0xFF; b = 0xFF; break;
  }

  auto fb = boot::BootInfo::the().get_framebuffer_info();
  
  // If we have precise mask info, use it (GOP/Multiboot2)
  if (fb.red_mask > 0) {
      uint32_t pixel = 0;
      pixel |= ((r >> (8 - fb.red_mask)) << fb.red_pos);
      pixel |= ((g >> (8 - fb.green_mask)) << fb.green_pos);
      pixel |= ((b >> (8 - fb.blue_mask)) << fb.blue_pos);
      return pixel;
  }

  // Default to RGB888
  return (r << 16) | (g << 8) | b;
}

void display_framebuffer::render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color,
                               uint32_t bg_color) {
  if (!framebuffer) return;

  const Vga::Font &font = Vga::default_font;
  if (!font.data) return;

  if (c < font.first_char || c > font.last_char) c = '?';
  
  uint32_t char_index = c - font.first_char;
  const uint8_t *glyph = font.data + (char_index * font.height);

  for (uint32_t row = 0; row < font.height; ++row) {
    uint8_t bits = glyph[row];
    for (uint32_t col = 0; col < font.width; ++col) {
      uint32_t px = x + col;
      uint32_t py = y + row;

      if (px >= fb_width || py >= fb_height) continue;

      uint32_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;
      uint32_t offset = py * fb_pitch + px * (fb_bpp / 8);

      if (fb_bpp == 32) {
          *reinterpret_cast<uint32_t *>(framebuffer + offset) = color;
      } else if (fb_bpp == 24) {
          framebuffer[offset] = color & 0xFF;
          framebuffer[offset + 1] = (color >> 8) & 0xFF;
          framebuffer[offset + 2] = (color >> 16) & 0xFF;
      }
    }
  }
}

void display_framebuffer::scroll() {
  if (!framebuffer) return;

  const Vga::Font &font = Vga::default_font;
  uint32_t max_rows = get_height();
  if (cursor_y < max_rows) return;

  uint32_t row_height = font.height;
  uint32_t scroll_bytes = row_height * fb_pitch;
  uint32_t total_bytes = fb_height * fb_pitch;

  if (scroll_bytes < total_bytes) {
      memmove(framebuffer, framebuffer + scroll_bytes, total_bytes - scroll_bytes);
  }

  // Clear bottom row
  uint32_t bg_pixel = color_to_pixel(current_bg);
  for (uint32_t y = (max_rows - 1) * row_height; y < fb_height; ++y) {
    for (uint32_t x = 0; x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(framebuffer + offset) = bg_pixel;
      } else if (fb_bpp == 24) {
        framebuffer[offset] = bg_pixel & 0xFF;
        framebuffer[offset + 1] = (bg_pixel >> 8) & 0xFF;
        framebuffer[offset + 2] = (bg_pixel >> 16) & 0xFF;
      }
    }
  }

  cursor_y = max_rows - 1;
}

void display_framebuffer::put_char(char c) {
  if (!framebuffer) {
    return;
  }

  // Handle standard control characters
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
    scroll();
    return;
  }

  if (c == '\r') {
    cursor_x = 0;
    return;
  }

  if (c == '\t') {
    uint32_t next_tab = (cursor_x + 8) & ~7;
    while (cursor_x < next_tab && cursor_x < get_width()) {
        render_char(cursor_x * Vga::default_font.width, 
                    cursor_y * Vga::default_font.height, 
                    ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
        cursor_x++;
    }
    if (cursor_x >= get_width()) {
        cursor_x = 0;
        cursor_y++;
        scroll();
    }
    return;
  }

  if (c == '\b') {
    if (cursor_x > 0) {
      cursor_x--;
      render_char(cursor_x * Vga::default_font.width, 
                  cursor_y * Vga::default_font.height, 
                  ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
    }
    return;
  }

  // Handle space character explicitly to ensure it always advances the cursor and clears the spot
  if (c == ' ') {
    render_char(cursor_x * Vga::default_font.width, 
                cursor_y * Vga::default_font.height, 
                ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
    cursor_x++;
    if (cursor_x >= get_width()) {
      cursor_x = 0;
      cursor_y++;
      scroll();
    }
    return;
  }

  // Handle line wrapping for other characters
  if (cursor_x >= get_width()) {
    cursor_x = 0;
    cursor_y++;
    scroll();
  }

  // Filter ONLY control characters below 32 (except ESC for ANSI)
  if (static_cast<unsigned char>(c) < 32 && static_cast<unsigned char>(c) != 27) {
    return;
  }

  render_char(cursor_x * Vga::default_font.width, 
              cursor_y * Vga::default_font.height, 
              c, color_to_pixel(current_fg), color_to_pixel(current_bg));

  cursor_x++;
}

void display_framebuffer::write(const char *str) {
  for (size_t i = 0; str[i]; ++i) {
    put_char(str[i]);
  }
}

void display_framebuffer::clear() {
  if (!framebuffer) {
    return;
  }

  uint32_t bg_pixel = color_to_pixel(current_bg);
  fk::algorithms::klog("DISPLAY", "Clearing screen with color 0x%x", bg_pixel);
  
  for (uint32_t y = 0; y < fb_height; ++y) {
    for (uint32_t x = 0; x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(framebuffer + offset) = bg_pixel;
      } else if (fb_bpp == 24) {
        framebuffer[offset] = bg_pixel & 0xFF;
        framebuffer[offset + 1] = (bg_pixel >> 8) & 0xFF;
        framebuffer[offset + 2] = (bg_pixel >> 16) & 0xFF;
      }
    }
  }

  cursor_x = 0;
  cursor_y = 0;
}

void display_framebuffer::set_color(Color fg, Color bg) {
  current_fg = fg;
  current_bg = bg;
}

void display_framebuffer::write_ansi(const char *str) {
  write_ansi_n(str, strlen(str));
}

void display_framebuffer::write_ansi_n(const char *str, size_t size) {
  size_t i = 0;
  while (i < size) {
    if (str[i] == '\033' && (i + 1 < size) && str[i + 1] == '[') {
      i += 2;
      
      int params[4] = {0, 0, 0, 0};
      int param_count = 0;
      
      while (i < size && ((str[i] >= '0' && str[i] <= '9') || str[i] == ';')) {
          if (str[i] == ';') {
              if (param_count < 3) param_count++;
              i++;
              continue;
          }
          params[param_count] = params[param_count] * 10 + (str[i] - '0');
          i++;
      }
      
      if (param_count > 0 || params[0] != 0) param_count++;
      else if (i < size && str[i] != ';' ) param_count = 1;

      char command = (i < size) ? str[i] : 0;
      if (i < size) i++;

      if (command == 'm') {
          Color current_fg_color = current_fg;
          Color current_bg_color = current_bg;
          
          for (int p = 0; p < (param_count == 0 ? 1 : param_count); ++p) {
              int code = params[p];
              switch (code) {
              case 0: current_fg_color = Color::LightGray; current_bg_color = Color::Black; break;
              case 1: // Bold
                  if (current_fg_color < Color::DarkGray)
                      current_fg_color = static_cast<Color>(static_cast<int>(current_fg_color) + 8);
                  break;
              case 30: current_fg_color = Color::Black; break;
              case 31: current_fg_color = Color::Red; break;
              case 32: current_fg_color = Color::Green; break;
              case 33: current_fg_color = Color::Brown; break;
              case 34: current_fg_color = Color::Blue; break;
              case 35: current_fg_color = Color::Magenta; break;
              case 36: current_fg_color = Color::Cyan; break;
              case 37: current_fg_color = Color::White; break;
              case 40: current_bg_color = Color::Black; break;
              case 41: current_bg_color = Color::Red; break;
              case 42: current_bg_color = Color::Green; break;
              case 43: current_bg_color = Color::Brown; break;
              case 44: current_bg_color = Color::Blue; break;
              case 45: current_bg_color = Color::Magenta; break;
              case 46: current_bg_color = Color::Cyan; break;
              case 47: current_bg_color = Color::White; break;
              }
          }
          set_color(current_fg_color, current_bg_color);
      } else if (command == 'J') {
          if (params[0] == 2) {
              clear();
          }
      } else if (command == 'H' || command == 'f') {
          cursor_y = (params[0] > 0) ? params[0] - 1 : 0;
          cursor_x = (params[1] > 0) ? params[1] - 1 : 0;
          if (cursor_y >= get_height()) cursor_y = get_height() - 1;
          if (cursor_x >= get_width()) cursor_x = get_width() - 1;
      }
    } else {
      put_char(str[i]);
      ++i;
    }
  }
}
