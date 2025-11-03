#include <Kernel/Boot/multiboot2.h>
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

// A very small 8x8 bitmap font for ASCII 32..127. For brevity we'll include
// a minimal subset for printable characters used by kernel messages (space,
// digits, letters and basic punctuation). Missing glyphs render as blank.
static const uint8_t font8x8_basic[96][8] = {
    // space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // '!' .. '0'..'9' etc. (we'll include digits and a few letters)
    // '!' (33)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},
    // '"' (34)
    {0x6C, 0x6C, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00},
    // '#' (35)
    {0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00},
    // '$' (36)
    {0x18, 0x3E, 0x58, 0x3C, 0x1A, 0x7C, 0x18, 0x00},
    // '%' (37)
    {0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xCC, 0x00},
    // '&' (38)
    {0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00},
    // '\'' (39)
    {0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00},
    // '(' (40)
    {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00},
    // ')' (41)
    {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00},
    // '*' (42)
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},
    // '+' (43)
    {0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00},
    // ',' (44)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30},
    // '-' (45)
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    // '.' (46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    // '/' (47)
    {0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00},
    // '0'..'9' (48..57)
    {0x7C, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00},
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    {0x7C, 0xC6, 0x06, 0x1C, 0x30, 0x66, 0xFE, 0x00},
    {0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00},
    {0x0C, 0x1C, 0x3C, 0x6C, 0xFE, 0x0C, 0x1E, 0x00},
    {0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00},
    {0x3C, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00},
    {0xFE, 0xC6, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
    {0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00},
    {0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00},
    // ':'..'@' (58..64) - blank/minimal
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0}};

void vga::fb_put_pixel(uint32_t x, uint32_t y, uint32_t rgba) {
  if (x >= fb_width || y >= fb_height || fb_addr == 0)
    return;

  uint8_t *ptr =
      reinterpret_cast<uint8_t *>(fb_addr) + y * fb_pitch + x * (fb_bpp / 8);
  // Support common 32bpp and 24bpp packed RGB
  if (fb_bpp == 32) {
    *reinterpret_cast<uint32_t *>(ptr) = rgba;
  } else if (fb_bpp == 24) {
    ptr[0] = static_cast<uint8_t>(rgba & 0xFF);
    ptr[1] = static_cast<uint8_t>((rgba >> 8) & 0xFF);
    ptr[2] = static_cast<uint8_t>((rgba >> 16) & 0xFF);
  }
}

uint32_t vga::color_to_rgba(Color fg,
                            [[maybe_unused]] Color bg) const noexcept {
  // Map a few colors to reasonable RGB values (ARGB/packed into 32-bit)
  switch (fg) {
  case Color::Black:
    return 0x00000000;
  case Color::White:
    return 0x00FFFFFF;
  case Color::Red:
    return 0x00FF0000;
  case Color::Green:
    return 0x0000FF00;
  case Color::Blue:
    return 0x000000FF;
  case Color::LightGray:
    return 0x00C0C0C0;
  case Color::DarkGray:
    return 0x00808080;
  case Color::Yellow:
    return 0x00FFFF00;
  default:
    return 0x00C0C0C0;
  }
}

void vga::fb_put_char(char c) {
  if (c == '\n') {
    col = 0;
    ++row;
    // simple scrolling: clear screen when bottom reached
    if (row * 8 >= fb_height) {
      fb_clear();
      row = 0;
    }
    return;
  }

  unsigned char uc = static_cast<unsigned char>(c);
  if (uc < 32 || uc >= 128) {
    ++col; // unsupported glyph
    return;
  }

  const uint8_t *glyph = font8x8_basic[c - 32];
  uint32_t fg = color_to_rgba(static_cast<Color>(color & 0xF),
                              static_cast<Color>((color >> 4) & 0xF));
  uint32_t bg =
      color_to_rgba(static_cast<Color>((color >> 4) & 0xF), Color::Black);

  uint32_t glyph_x = static_cast<uint32_t>(col * 8);
  uint32_t glyph_y = static_cast<uint32_t>(row * 8);

  for (uint32_t ry = 0; ry < 8; ++ry) {
    uint8_t bits = glyph[ry];
    for (uint32_t rx = 0; rx < 8; ++rx) {
      bool on = bits & (1 << (7 - rx));
      fb_put_pixel(glyph_x + rx, glyph_y + ry, on ? fg : bg);
    }
  }

  ++col;
  if ((col + 1) * 8 > fb_width) {
    col = 0;
    ++row;
    if (row * 8 >= fb_height) {
      fb_clear();
      row = 0;
    }
  }
}

void vga::fb_clear() {
  if (fb_addr == 0)
    return;
  for (uint32_t y = 0; y < fb_height; ++y) {
    for (uint32_t x = 0; x < fb_width; ++x) {
      fb_put_pixel(x, y, color_to_rgba(Color::LightGray, Color::Black));
    }
  }
}

bool vga::initialize_framebuffer(
    const multiboot2::TagFramebuffer *fb_tag) noexcept {
  if (!fb_tag)
    return false;
  // Basic validation
  if (fb_tag->framebuffer_addr == 0 || fb_tag->framebuffer_width == 0 ||
      fb_tag->framebuffer_height == 0)
    return false;

  fb_addr = static_cast<uintptr_t>(fb_tag->framebuffer_addr);
  fb_width = fb_tag->framebuffer_width;
  fb_height = fb_tag->framebuffer_height;
  fb_pitch = fb_tag->framebuffer_pitch;
  fb_bpp = fb_tag->framebuffer_bpp;
  fb_type = fb_tag->framebuffer_type;

  mode = Mode::Framebuffer;
  // Reset cursor state to grid based on 8x8 font
  row = 0;
  col = 0;
  fb_clear();
  return true;
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
