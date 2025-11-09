#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

vga::vga() : row(0), col(0), color(vga_entry_color(Color::LightGray, Color::Black)) {
  enable_cursor();
  update_cursor();
}

void vga::update_cursor() {
  uint16_t pos = static_cast<uint16_t>(row * VGA_WIDTH + col);

  outb(0x3D4, 0x0F);
  outb(0x3D5, static_cast<uint8_t>(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

void vga::enable_cursor() {
  outb(0x3D4, 0x0A); // CRTC Register 0x0A: Cursor Start Register
  outb(0x3D5, (inb(0x3D5) & 0xC0) | 6); // Set cursor start scanline (e.g., 6) and enable cursor (bit 5 = 0)

  outb(0x3D4, 0x0B); // CRTC Register 0x0B: Cursor End Register
  outb(0x3D5, (inb(0x3D5) & 0xE0) | 7); // Set cursor end scanline (e.g., 7)
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
}

void vga::write(const char *str) {
  for (size_t i = 0; str[i]; ++i) {
    put_char(str[i]);
  }
}

void vga::clear() {
  for (size_t r = 0; r < VGA_HEIGHT; ++r) {
    for (size_t c = 0; c < VGA_WIDTH; ++c) {
      text_buffer[r * VGA_WIDTH + c] = vga_entry(' ', color);
    }
  }
  row = 0;
  col = 0;
  update_cursor();
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
