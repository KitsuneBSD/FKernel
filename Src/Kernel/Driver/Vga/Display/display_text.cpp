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

void DisplayText::put_codepoint(uint32_t codepoint) {
    put_char(static_cast<char>(codepoint));
}

void DisplayText::put_char(char c) {
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
      buffer[row * WIDTH + col] = (static_cast<uint16_t>(color) << 8) | ' ';
      update_cursor();
    }
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
  for (size_t r = 0; r < HEIGHT; ++r) {
    for (size_t c = 0; c < WIDTH; ++c) {
      buffer[r * WIDTH + c] = (static_cast<uint16_t>(color) << 8) | ' ';
    }
  }
  row = 0;
  col = 0;
  update_cursor();
}

void DisplayText::write_ansi(const char *str) {
  write_ansi_n(str, strlen(str));
}

void DisplayText::write_ansi_n(const char *str, size_t size) {
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
      else if (i < size && str[i] != ';' ) param_count = 1; // case like [m or [H

      char command = (i < size) ? str[i] : 0;
      if (i < size) i++;

      if (command == 'm') {
          Color current_fg = static_cast<Color>(color & 0x0F);
          Color current_bg = static_cast<Color>((color >> 4) & 0x0F);
          
          for (int p = 0; p < (param_count == 0 ? 1 : param_count); ++p) {
              int code = params[p];
              switch (code) {
              case 0: current_fg = Color::LightGray; current_bg = Color::Black; break;
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
          }
          set_color(current_fg, current_bg);
      } else if (command == 'J') {
          if (params[0] == 2) {
              clear();
          }
      } else if (command == 'K') {
          // Clear line from cursor to end
          for (size_t c = col; c < WIDTH; ++c) {
              buffer[row * WIDTH + c] = (static_cast<uint16_t>(color) << 8) | ' ';
          }
      } else if (command == 'H' || command == 'f') {
          row = (params[0] > 0) ? params[0] - 1 : 0;
          col = (params[1] > 0) ? params[1] - 1 : 0;
          if (row >= HEIGHT) row = HEIGHT - 1;
          if (col >= WIDTH) col = WIDTH - 1;
          update_cursor();
      } else if (command == 'A') { // Cursor Up
          int n = (params[0] == 0) ? 1 : params[0];
          if (row >= (size_t)n) row -= n; else row = 0;
          update_cursor();
      } else if (command == 'B') { // Cursor Down
          int n = (params[0] == 0) ? 1 : params[0];
          row += n; if (row >= HEIGHT) row = HEIGHT - 1;
          update_cursor();
      } else if (command == 'C') { // Cursor Forward
          int n = (params[0] == 0) ? 1 : params[0];
          col += n; if (col >= WIDTH) col = WIDTH - 1;
          update_cursor();
      } else if (command == 'D') { // Cursor Backward
          int n = (params[0] == 0) ? 1 : params[0];
          if (col >= (size_t)n) col -= n; else col = 0;
          update_cursor();
      }
    } else {
      uint32_t codepoint = 0;
      uint8_t c = static_cast<uint8_t>(str[i]);

      if (c <= 0x7F) {
          codepoint = c;
          i += 1;
      } else if ((c & 0xE0) == 0xC0 && i + 1 < size) {
          codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F);
          i += 2;
      } else if ((c & 0xF0) == 0xE0 && i + 2 < size) {
          codepoint = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+2]) & 0x3F);
          i += 3;
      } else if ((c & 0xF8) == 0xF0 && i + 3 < size) {
          codepoint = ((c & 0x07) << 18) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) | ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+3]) & 0x3F);
          i += 4;
      } else {
          codepoint = '?';
          i += 1;
      }
      put_codepoint(codepoint);
    }
  }
}