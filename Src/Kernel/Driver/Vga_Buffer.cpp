#include <Driver/Vga_Buffer.hpp>

uint16_t vga::Console::make_entry(char c) const {
  return (static_cast<uint16_t>(color) << 8) | c;
}

void vga::Console::putchar_raw(char c, size_t col, size_t row) {
  const size_t index = row * VGA_WIDTH + col;
  buffer[index] = make_entry(c);
}

void vga::Console::new_line() {
  column = 0;
  if (++row == VGA_HEIGHT) {
    scroll();
    row = VGA_HEIGHT - 1;
  }
}

void vga::Console::scroll() {
  for (size_t y = 1; y < VGA_HEIGHT; ++y) {
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
      buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
    }
  }
  for (size_t x = 0; x < VGA_WIDTH; ++x) {
    buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ');
  }
}

void vga::Console::update_cursor() {
  uint16_t pos = row * VGA_WIDTH + column;
  outb(0x3D4, 0x0F);
  outb(0x3D5, pos & 0xFF);
  outb(0x3D4, 0x0E);
  outb(0x3D5, (pos >> 8) & 0xFF);
}

void vga::Console::clear() {
  for (size_t y = 0; y < VGA_HEIGHT; ++y) {
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
      putchar_raw(' ', x, y);
    }
  }
  row = column = 0;
  update_cursor();
}

void vga::Console::set_color(Color fg, Color bg) {
  color = encode_color(fg, bg);
}

void vga::Console::putchar(char c) {
  if (c == '\n') {
    new_line();
    update_cursor();
    return;
  }

  putchar_raw(c, column, row);
  if (++column == VGA_WIDTH) {
    new_line();
  }

  update_cursor();
}

void vga::Console::write_hex(uint64_t value, bool prefix, bool uppercase) {
  if (prefix) {
    write("0x");
  }

  if (value == 0) {
    write("0");
    new_line();
    return;
  }

  char buffer[17];
  buffer[16] = '\0';
  int pos = 15;

  while (value != 0 && pos >= 0) {
    uint8_t digit = value & 0xF;
    buffer[pos--] =
        (digit < 10) ? ('0' + digit) : ((uppercase ? 'A' : 'a') + (digit - 10));
    value >>= 4;
  }

  write(&buffer[pos + 1]);
  new_line();
}

void vga::Console::write_dec(uint64_t value) {
  char buffer[20];
  int i = 19;
  buffer[i--] = '\0';

  if (value == 0) {
    write("0");
    return;
  }

  while (value > 0 && i >= 0) {
    buffer[i--] = '0' + (value % 10);
    value /= 10;
  }

  write(&buffer[i + 1]);
}

void vga::Console::write(const char *str) {
  while (*str)
    putchar(*str++);
}
