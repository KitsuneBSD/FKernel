#include <Kernel/Driver/Vga/display.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

DisplayText::DisplayText() : row(0), col(0), color(0x07) {
  enable_cursor();
  update_cursor();
}

void DisplayText::update_cursor() {
  uint16_t pos = static_cast<uint16_t>(row * WIDTH + col);

  outb(0x3D4, 0x0F);
  outb(0x3D5, static_cast<uint8_t>(pos & 0xFF));

  outb(0x3D4, 0x0E);
  outb(0x3D5, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

void DisplayText::enable_cursor() {
  outb(0x3D4, 0x0A);
  outb(0x3D5, (inb(0x3D5) & 0xC0) | 6);

  outb(0x3D4, 0x0B);
  outb(0x3D5, (inb(0x3D5) & 0xE0) | 7);
}

void DisplayText::scroll() {
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

void DisplayText::set_color(Color fg, Color bg) {
  color = static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

void DisplayText::set_cursor_pos(uint32_t x, uint32_t y) {
    col = x;
    row = y;
    if (col >= WIDTH) col = WIDTH - 1;
    if (row >= HEIGHT) row = HEIGHT - 1;
    update_cursor();
}

void DisplayText::show_cursor(bool visible) {
    if (visible) enable_cursor();
    else {
        // VGA doesn't have a simple "hide" bit without risk, 
        // a common trick is to move it off-screen
        outb(0x3D4, 0x0F);
        outb(0x3D5, 0xFF);
        outb(0x3D4, 0x0E);
        outb(0x3D5, 0xFF);
    }
}

void DisplayText::put_codepoint(uint32_t codepoint) {
    put_char(static_cast<char>(codepoint));
}

void DisplayText::put_char(char c) {
  fk::synchronization::ScopedLock lock(Display::lock());
  if (c == '\n') {
    col = 0;
    ++row;
    scroll();
    update_cursor();
    return;
  }

  if (c == '\r') {
    col = 0;
    update_cursor();
    return;
  }

  if (c == '\t') {
    col = (col + 8) & ~7;
    if (col >= WIDTH) {
      col = 0;
      ++row;
      scroll();
    }
    update_cursor();
    return;
  }

  if (c == '\b') {
    if (col > 0) {
      --col;
    } else if (row > 0) {
      --row;
      col = WIDTH - 1;
    } else {
      return;
    }
    buffer[row * WIDTH + col] = (static_cast<uint16_t>(color) << 8) | ' ';
    update_cursor();
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

void DisplayText::write(const char *str) {
  size_t i = 0;
  while (str[i]) {
      uint32_t codepoint = 0;
      uint8_t c = static_cast<uint8_t>(str[i]);

      if (c <= 0x7F) {
          codepoint = c;
          i += 1;
      } else if ((c & 0xE0) == 0xC0) {
          codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F);
          i += 2;
      } else if ((c & 0xF0) == 0xE0) {
          codepoint = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+2]) & 0x3F);
          i += 3;
      } else if ((c & 0xF8) == 0xF0) {
          codepoint = ((c & 0x07) << 18) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) | ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+3]) & 0x3F);
          i += 4;
      } else {
          codepoint = '?';
          i += 1;
      }
      put_codepoint(codepoint);
  }
}

void DisplayText::clear() {
  clear_rect(0, 0, WIDTH, HEIGHT);
  row = 0;
  col = 0;
  update_cursor();
}

void DisplayText::clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLock lock(Display::lock());
  for (size_t r = y; r < y + height && r < HEIGHT; ++r) {
    for (size_t c = x; c < x + width && c < WIDTH; ++c) {
      buffer[r * WIDTH + c] = (static_cast<uint16_t>(color) << 8) | ' ';
    }
  }
}

void DisplayText::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLock lock(Display::lock());
  // Simplified: only support full-width line copying for now as used by terminal
  if (width == WIDTH && src_x == 0 && dst_x == 0) {
      memmove((void*)&buffer[dst_y * WIDTH], (void*)&buffer[src_y * WIDTH], height * WIDTH * 2);
  }
}

void DisplayText::write_ansi(const char *str) {
  write_ansi_n(str, strlen(str));
}

void DisplayText::write_ansi_n(const char *str, size_t size) {
  // Use the common put_char which handles scrolling and cursor
  for (size_t i = 0; i < size; ++i) {
      put_char(str[i]);
  }
}