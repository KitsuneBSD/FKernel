#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

// ============================================================================
// Global display backend selection
// ============================================================================

display &display::the() {
  // Check if we're booting via EFI
  if (boot::BootInfo::the().is_efi_boot()) {
    return display_efi::the();
  }
  // Fall back to BIOS text mode
  return display_text::the();
}

// ============================================================================
// BIOS Text-Mode Display Implementation
// ============================================================================

display_text::display_text() : row(0), col(0), color(0x07) {
  enable_cursor();
  update_cursor();
}

void display_text::update_cursor() {
  uint16_t pos = static_cast<uint16_t>(row * WIDTH + col);

  outb(0x3D4, 0x0F);
  outb(0x3D5, static_cast<uint8_t>(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

void display_text::enable_cursor() {
  outb(0x3D4, 0x0A);
  outb(0x3D5, (inb(0x3D5) & 0xC0) | 6);

  outb(0x3D4, 0x0B);
  outb(0x3D5, (inb(0x3D5) & 0xE0) | 7);
}

void display_text::scroll() {
  if (row < HEIGHT)
    return;

  for (size_t r = 1; r < HEIGHT; ++r) {
    for (size_t c = 0; c < WIDTH; ++c) {
      buffer[(r - 1) * WIDTH + c] = buffer[r * WIDTH + c];
    }
  }

  for (size_t c = 0; c < WIDTH; ++c) {
    buffer[(HEIGHT - 1) * WIDTH + c] = (static_cast<uint16_t>(color) << 8) | ' ';
  }

  row = HEIGHT - 1;
}

void display_text::set_color(Color fg, Color bg) {
  color = static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

void display_text::put_char(char c) {
  if (c == '\n') {
    col = 0;
    ++row;
    scroll();
    return;
  }

  buffer[row * WIDTH + col] = (static_cast<uint16_t>(color) << 8) | static_cast<uint8_t>(c);
  ++col;
  if (col >= WIDTH) {
    col = 0;
    ++row;
    scroll();
  }

  update_cursor();
}

void display_text::write(const char *str) {
  for (size_t i = 0; str[i]; ++i) {
    put_char(str[i]);
  }
}

void display_text::clear() {
  for (size_t r = 0; r < HEIGHT; ++r) {
    for (size_t c = 0; c < WIDTH; ++c) {
      buffer[r * WIDTH + c] = (static_cast<uint16_t>(color) << 8) | ' ';
    }
  }
  row = 0;
  col = 0;
  update_cursor();
}

void display_text::write_ansi(const char *str) {
  Color current_fg = Color::LightGray;
  Color current_bg = Color::Black;

  size_t i = 0;
  while (str[i]) {
    if (str[i] == '\033' && str[i + 1] == '[') {
      i += 2;
      int code = 0;
      while (str[i] >= '0' && str[i] <= '9') {
        code = code * 10 + (str[i] - '0');
        ++i;
      }
      if (str[i] == 'm') {
        switch (code) {
        case 0:
          current_fg = Color::LightGray;
          current_bg = Color::Black;
          break;
        case 30: current_fg = Color::Black; break;
        case 31: current_fg = Color::Red; break;
        case 32: current_fg = Color::Green; break;
        case 33: current_fg = Color::Brown; break;
        case 34: current_fg = Color::Blue; break;
        case 35: current_fg = Color::Magenta; break;
        case 36: current_fg = Color::Cyan; break;
        case 37: current_fg = Color::White; break;
        case 40: current_bg = Color::Black; break;
        case 41: current_bg = Color::Red; break;
        case 42: current_bg = Color::Green; break;
        case 43: current_bg = Color::Brown; break;
        case 44: current_bg = Color::Blue; break;
        case 45: current_bg = Color::Magenta; break;
        case 46: current_bg = Color::Cyan; break;
        case 47: current_bg = Color::White; break;
        }
        set_color(current_fg, current_bg);
      }
      ++i;
    } else {
      put_char(str[i]);
      ++i;
    }
  }
}

// ============================================================================
// EFI Framebuffer Display Implementation
// ============================================================================

display_efi::display_efi() : cursor_x(0), cursor_y(0) {
  initialize_framebuffer();
  clear();
}

void display_efi::initialize_framebuffer() {
  // Initialize framebuffer from BootInfo (multiboot2 framebuffer tag)
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    framebuffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(fb.addr));
    fb_width = fb.width;
    fb_height = fb.height;
    fb_pitch = fb.pitch;
    fb_bpp = fb.bpp;
  } else {
    // Fall back to reasonable defaults when no framebuffer tag is present
    framebuffer = nullptr;
    fb_width = 1024;
    fb_height = 768;
    fb_pitch = 4096;
    fb_bpp = 32;
  }
}

uint32_t display_efi::color_to_pixel(Color c) const {
  // Convert VGA color to RGB pixel value
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

  // Pack into pixel format (assume RGB888 or similar)
  if (fb_bpp == 32) {
    return (r << 16) | (g << 8) | b;
  } else if (fb_bpp == 24) {
    return (r << 16) | (g << 8) | b;
  }
  return 0;
}

void display_efi::render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color,
                               uint32_t bg_color) {
  if (!framebuffer) {
    return;
  }

  // Use default font
  const Vga::Font &font = Vga::default_font;
  if (!font.data) {
    return;
  }

  // Ensure character is in valid range
  if (c < font.first_char || c > font.last_char) {
    c = '?';
  }
  
  // Get glyph data for the character
  uint32_t char_index = c - font.first_char;
  const uint8_t *glyph = font.data + (char_index * font.height);

  // Render character to framebuffer
  for (uint32_t row = 0; row < font.height; ++row) {
    uint8_t bits = glyph[row];
    
    for (uint32_t col = 0; col < font.width; ++col) {
      uint32_t px = x + col;
      uint32_t py = y + row;

      if (px >= fb_width || py >= fb_height) {
        continue;
      }

      uint32_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;
      uint32_t offset = py * fb_pitch + px * (fb_bpp / 8);

      if (offset + (fb_bpp / 8) <= fb_pitch * fb_height && framebuffer) {
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
}

void display_efi::scroll() {
  if (!framebuffer) {
    return;
  }

  const Vga::Font &font = Vga::default_font;
  uint32_t max_rows = get_height();
  if (cursor_y < max_rows) {
    return;
  }

  // Scroll framebuffer up by one character height
  uint32_t scroll_bytes = font.height * fb_pitch;
  uint32_t copy_bytes = (max_rows - 1) * font.height * fb_pitch;

  memmove(framebuffer, framebuffer + scroll_bytes, copy_bytes);

  // Clear bottom line
  uint32_t bg_pixel = color_to_pixel(current_bg);
  
  for (uint32_t y = (max_rows - 1) * font.height; y < max_rows * font.height; ++y) {
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

void display_efi::put_char(char c) {
  if (!framebuffer) {
    return;
  }

  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
    scroll();
    return;
  }

  uint32_t fg_pixel = color_to_pixel(current_fg);
  uint32_t bg_pixel = color_to_pixel(current_bg);

  render_char(cursor_x * CHAR_WIDTH, cursor_y * CHAR_HEIGHT, c, fg_pixel, bg_pixel);

  cursor_x++;
  if (cursor_x >= get_width()) {
    cursor_x = 0;
    cursor_y++;
    scroll();
  }
}

void display_efi::write(const char *str) {
  for (size_t i = 0; str[i]; ++i) {
    put_char(str[i]);
  }
}

void display_efi::clear() {
  if (!framebuffer) {
    return;
  }

  uint32_t bg_pixel = color_to_pixel(current_bg);
  
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

void display_efi::set_color(Color fg, Color bg) {
  current_fg = fg;
  current_bg = bg;
}

void display_efi::write_ansi(const char *str) {
  Color current_fg_color = current_fg;
  Color current_bg_color = current_bg;

  size_t i = 0;
  while (str[i]) {
    if (str[i] == '\033' && str[i + 1] == '[') {
      i += 2;
      int code = 0;
      while (str[i] >= '0' && str[i] <= '9') {
        code = code * 10 + (str[i] - '0');
        ++i;
      }
      if (str[i] == 'm') {
        switch (code) {
        case 0:
          current_fg_color = Color::LightGray;
          current_bg_color = Color::Black;
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
        set_color(current_fg_color, current_bg_color);
      }
      ++i;
    } else {
      put_char(str[i]);
      ++i;
    }
  }
}
