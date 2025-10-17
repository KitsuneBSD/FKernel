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
      buffer[(r - 1) * VGA_WIDTH + c] = buffer[r * VGA_WIDTH + c];
    }
  }

  for (size_t c = 0; c < VGA_WIDTH; ++c) {
    buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + c] = vga_entry(' ', color);
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

  buffer[row * VGA_WIDTH + col] = vga_entry(c, color);
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
      buffer[r * VGA_WIDTH + c] = vga_entry(' ', color);
    }
  }
  row = 0;
  col = 0;
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
