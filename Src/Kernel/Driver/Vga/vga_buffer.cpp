#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

void vga::update_cursor() {
  uint16_t pos = static_cast<uint16_t>(row * VGA_WIDTH + col);

  outb(0x3D4, 0x0F);
  outb(0x3D5, static_cast<uint8_t>(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

void vga::scroll() {
  if (row < VGA_HEIGHT)
    return;

  for (size_t r = 1; r < VGA_HEIGHT; ++r) {
    for (size_t c = 0; c < VGA_WIDTH; ++c) {
      text_buffer[(r - 1) * VGA_WIDTH + c] = text_buffer[r * VGA_WIDTH + c];
    }
  }

  for (size_t c = 0; c < VGA_WIDTH; ++c) {
    text_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + c] = vga_entry(' ', color);
  }

  row = VGA_HEIGHT - 1;
}

void vga::set_color(Color fg, Color bg) { color = vga_entry_color(fg, bg); }

void vga::put_char(char c) {
  if (mode == Mode::Text) {
    if (c == '\n') {
      col = 0;
      ++row;
      scroll();
      return;
    }

    text_buffer[row * VGA_WIDTH + col] = vga_entry(c, color);
    ++col;
    if (col >= VGA_WIDTH) {
      col = 0;
      ++row;
      scroll();
    }

    update_cursor();
    return;
  }

  // Framebuffer mode
  fb_put_char(c);
}

void vga::write(const char *str) {
  for (size_t i = 0; str[i]; ++i) {
    put_char(str[i]);
  }
}

void vga::clear() {
  if (mode == Mode::Text) {
    for (size_t r = 0; r < VGA_HEIGHT; ++r) {
      for (size_t c = 0; c < VGA_WIDTH; ++c) {
        text_buffer[r * VGA_WIDTH + c] = vga_entry(' ', color);
      }
    }
    row = 0;
    col = 0;
    update_cursor();
    return;
  }

  fb_clear();
}

/**
 * @brief Convert a VGA color to an RGBA uint32_t value, scaled and positioned
 * for the framebuffer.
 */
uint32_t vga::get_rgba_for_vga_color(Color vga_color) const noexcept {
  // Get the base RGBA value for the VGA color (assuming 8 bits per channel)
  uint32_t base_rgba = vga_color_to_rgba[static_cast<uint8_t>(vga_color)];

  // Extract R, G, B components from the base RGBA (0-255 scale)
  uint8_t r_vga = (base_rgba >> 16) & 0xFF;
  uint8_t g_vga = (base_rgba >> 8) & 0xFF;
  uint8_t b_vga = (base_rgba >> 0) & 0xFF;

  // Construct the final RGBA value based on framebuffer format
  uint32_t final_red = 0, final_green = 0, final_blue = 0;

  // Scale and shift Red component
  if (fb_red_size > 0) {
    uint32_t red_max_val = (1 << fb_red_size) - 1;
    uint32_t scaled_red = (r_vga * red_max_val) / 255;
    final_red = scaled_red << fb_red_pos;
  }

  // Scale and shift Green component
  if (fb_green_size > 0) {
    uint32_t green_max_val = (1 << fb_green_size) - 1;
    uint32_t scaled_green = (g_vga * green_max_val) / 255;
    final_green = scaled_green << fb_green_pos;
  }

  // Scale and shift Blue component
  if (fb_blue_size > 0) {
    uint32_t blue_max_val = (1 << fb_blue_size) - 1;
    uint32_t scaled_blue = (b_vga * blue_max_val) / 255;
    final_blue = scaled_blue << fb_blue_pos;
  }

  // Combine the components.
  uint32_t final_rgba = final_red | final_green | final_blue;

  // Alpha handling: For simplicity, we assume opaque alpha (0xFF) if fb_bpp is
  // 32 and there's space for it. The exact placement depends on the framebuffer
  // format (e.g., ARGB, RGBA). This implementation assumes the `rgba` value
  // passed to `fb_put_pixel` is correctly formatted. If fb_bpp is 32 and the
  // sum of R, G, B sizes is less than 32, there's room for alpha. We'll rely on
  // the fact that the `final_rgba` construction will implicitly handle it or
  // that the `rgba` value passed to `fb_put_pixel` is already correctly
  // formatted. A more robust solution would explicitly determine alpha position
  // from fb_tag.

  return final_rgba;
}

/**
 * @brief Puts a single pixel on the framebuffer.
 */
void vga::fb_put_pixel(uint32_t x, uint32_t y, uint32_t color_data) {
  if (fb_addr == 0 || x >= fb_width || y >= fb_height || fb_pitch == 0 ||
      fb_bpp == 0)
    return;

  uintptr_t offset = static_cast<uintptr_t>(y) * fb_pitch +
                     static_cast<uintptr_t>(x) * (fb_bpp / 8);
  uint8_t *pixel_ptr = reinterpret_cast<uint8_t *>(fb_addr + offset);

  if (fb_type == 0) { // Indexed color
    if (fb_bpp == 8) {
      // color_data is the VGA color index (0-15)
      *reinterpret_cast<volatile uint8_t *>(pixel_ptr) = static_cast<uint8_t>(color_data);
    } else {
      // Unsupported indexed format, log error or handle appropriately
      // For now, do nothing.
    }
  } else if (fb_type == 1) { // RGB color
    uint8_t r = (color_data >> 16) & 0xFF;
    uint8_t g = (color_data >> 8) & 0xFF;
    uint8_t b = (color_data >> 0) & 0xFF;

    if (fb_bpp == 32) {
      uint32_t packed =
          pack_pixel_from_rgb(r, g, b, fb_red_pos, fb_red_size, fb_green_pos,
                              fb_green_size, fb_blue_pos, fb_blue_size);
      uint32_t total_rgb_bits = fb_red_size + fb_green_size + fb_blue_size;
      if (total_rgb_bits <= 24) {
        packed |= 0xFFu << 24;
      }
      *reinterpret_cast<volatile uint32_t *>(pixel_ptr) = packed;
    } else if (fb_bpp == 24) {
      uint32_t packed =
          pack_pixel_from_rgb(r, g, b, fb_red_pos, fb_red_size, fb_green_pos,
                              fb_green_size, fb_blue_pos, fb_blue_size);
      pixel_ptr[0] = static_cast<uint8_t>(packed & 0xFF);
      pixel_ptr[1] = static_cast<uint8_t>((packed >> 8) & 0xFF);
      pixel_ptr[2] = static_cast<uint8_t>((packed >> 16) & 0xFF);
    } else if (fb_bpp == 16) {
      uint32_t packed =
          pack_pixel_from_rgb(r, g, b, fb_red_pos, fb_red_size, fb_green_pos,
                              fb_green_size, fb_blue_pos, fb_blue_size) &
          0xFFFFu;
      *reinterpret_cast<volatile uint16_t *>(pixel_ptr) =
          static_cast<uint16_t>(packed);
    } else {
      if (fb_bpp <= 32) {
        uint32_t packed =
            pack_pixel_from_rgb(r, g, b, fb_red_pos, fb_red_size, fb_green_pos,
                                fb_green_size, fb_blue_pos, fb_blue_size);
        size_t bytes = fb_bpp / 8;
        for (size_t i = 0; i < bytes; ++i) {
          pixel_ptr[i] = static_cast<uint8_t>((packed >> (8 * i)) & 0xFF);
        }
      }
    }
  }
}

void vga::fb_put_char(char c) {
  if (c < Vga::default_font.first_char || c > Vga::default_font.last_char)
    c = '?';

  const uint8_t *glyph =
      Vga::default_font.data +
      (c - Vga::default_font.first_char) * Vga::default_font.height;

  for (uint8_t y = 0; y < Vga::default_font.height; ++y) {
    uint8_t line = glyph[y];
    for (uint8_t x = 0; x < Vga::default_font.width; ++x) {
      uint8_t fg_color_index = static_cast<uint8_t>(color & 0x0F);
      uint8_t bg_color_index = static_cast<uint8_t>((color >> 4) & 0x0F);

      uint32_t pixel_color_data;

      if (fb_type == 0) { // Indexed color
        uint8_t selected_color_index = (line & (1 << (7 - x))) ? fg_color_index : bg_color_index;
        pixel_color_data = selected_color_index; // Pass the index directly
      } else { // RGB color (fb_type == 1)
        uint32_t rgba = (line & (1 << (7 - x)))
                            ? get_rgba_for_vga_color(static_cast<Color>(fg_color_index))
                            : get_rgba_for_vga_color(static_cast<Color>(bg_color_index));
        pixel_color_data = rgba; // Pass the RGBA value
      }

      fb_put_pixel(col * Vga::default_font.width + x,
                   row * Vga::default_font.height + y, pixel_color_data);
    }
  }

  if (++col >= fb_width / Vga::default_font.width) {
    col = 0;
    ++row;
  }
}

void vga::fb_clear() {
  if (fb_addr == 0)
    return;

  if (fb_type == 0) { // Indexed color
    uint8_t clear_color_index = static_cast<uint8_t>(Color::Black); // Use Black for indexed clear
    for (uint32_t y = 0; y < fb_height; ++y) {
      for (uint32_t x = 0; x < fb_width; ++x) {
        fb_put_pixel(x, y, clear_color_index);
      }
    }
  } else { // RGB color (fb_type == 1)
    for (uint32_t y = 0; y < fb_height; ++y) {
      for (uint32_t x = 0; x < fb_width; ++x) {
        fb_put_pixel(x, y, get_rgba_for_vga_color(Color::LightGray));
      }
    }
  }
}

bool vga::initialize_framebuffer(const multiboot2::TagFramebuffer *fb_tag) noexcept {
  if (!fb_tag) {
    mode = Mode::Text; // No framebuffer tag, remain in text mode
    return false;
  }

  // Basic validation
  if (fb_tag->framebuffer_addr == 0 || fb_tag->framebuffer_width == 0 ||
      fb_tag->framebuffer_height == 0) {
    mode = Mode::Text; // Invalid framebuffer parameters, fall back to text mode
    return false;
  }

  fb_addr = static_cast<uintptr_t>(fb_tag->framebuffer_addr);
  fb_width = fb_tag->framebuffer_width;
  fb_height = fb_tag->framebuffer_height;
  fb_pitch = fb_tag->framebuffer_pitch;
  fb_bpp = fb_tag->framebuffer_bpp;
  fb_type = fb_tag->framebuffer_type;

  // Set mode based on framebuffer type
  // We explicitly support indexed (0) and RGB (1) framebuffers.
  // Other types (like EGA text mode, type 2) will fall back to text mode.
  if (fb_type == 0 || fb_type == 1) { // Indexed or RGB
    mode = Mode::Framebuffer; // Switch to framebuffer mode
  } else { // Unsupported framebuffer type, fall back to text mode
    mode = Mode::Text;
    // Note: EGA text mode might require different initialization if we were to support it fully.
    // For now, we assume graphical framebuffers are preferred, but will fall back if not supported.
    return false; // Indicate failure to initialize framebuffer mode
  }

  // Store color mask information for RGB framebuffers
  fb_red_pos = fb_tag->rgb.red_field_position;
  fb_red_size = fb_tag->rgb.red_mask_size;
  fb_green_pos = fb_tag->rgb.green_field_position;
  fb_green_size = fb_tag->rgb.green_mask_size;
  fb_blue_pos = fb_tag->rgb.blue_field_position;
  fb_blue_size = fb_tag->rgb.blue_mask_size;

  // Reset cursor state to grid based on 8x8 font
  row = 0;
  col = 0;
  fb_clear(); // Clear the screen in the newly initialized mode
  return true; // Indicate success in initializing framebuffer mode
}

void vga::write_ansi(const char *str) {
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
        case 30:
          current_fg = Color::Black;
          break;
        case 31:
          current_fg = Color::Red;
          break;
        case 32:
          current_fg = Color::Green;
          break;
        case 33:
          current_fg = Color::Brown;
          break;
        case 34:
          current_fg = Color::Blue;
          break;
        case 35:
          current_fg = Color::Magenta;
          break;
        case 36:
          current_fg = Color::Cyan;
          break;
        case 37:
          current_fg = Color::White;
          break;
        case 40:
          current_bg = Color::Black;
          break;
        case 41:
          current_bg = Color::Red;
          break;
        case 42:
          current_bg = Color::Green;
          break;
        case 43:
          current_bg = Color::Brown;
          break;
        case 44:
          current_bg = Color::Blue;
          break;
        case 45:
          current_bg = Color::Magenta;
          break;
        case 46:
          current_bg = Color::Cyan;
          break;
        case 47:
          current_bg = Color::White;
          break;
        }
        set_color(current_fg, current_bg);
      }
      ++i;
      continue;
    }

    put_char(str[i]);
    ++i;
  }
}
