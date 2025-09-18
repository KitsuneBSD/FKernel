#include <Kernel/Driver/Vga/vga_buffer.h>

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
