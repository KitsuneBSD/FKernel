#include "../../../../Include/Kernel/Driver/vga_buffer.h"
#include "../../../../Include/Kernel/LibK/port_io.h"

static const int MAX_VGA_COLS = 80;
static const int MAX_VGA_ROWS = 25;

#define VGA_PORT 0x3D4
#define VGA_DATA 0x3D5

int cursor_x = 0;
int cursor_y = 0;

uint8_t default_color = WHITE | BLACK << 4;

struct Char *vga_adress = (struct Char *)0xb8000;

void update_cursor() {
  uint16_t position = (cursor_y * MAX_VGA_COLS) + cursor_x;

  outb(VGA_PORT, 0x0E);
  outb(VGA_DATA, (position >> 8) & 0xFF);

  outb(VGA_PORT, 0x0F);
  outb(VGA_DATA, position & 0xFF);
}

void clear_row(size_t row) {
  struct Char empty = (struct Char){
      .character = ' ',
      .color = default_color,
  };

  for (size_t col = 0; col < MAX_VGA_COLS; ++col) {
    vga_adress[col + (MAX_VGA_COLS * row)] = empty;
  }
}

void clear_screen() {
  for (size_t i = 0; i < MAX_VGA_ROWS; ++i) {
    clear_row(i);
  }

  cursor_x = 0;
  cursor_y = 0;
  update_cursor();
}

void newline() {
  cursor_x = 0;

  if (cursor_y < MAX_VGA_ROWS - 1) {
    cursor_y++;
    return;
  }

  for (size_t row = 1; row < MAX_VGA_ROWS; ++row) {
    for (size_t col = 0; col < MAX_VGA_COLS; ++col) {
      struct Char character = vga_adress[col + (MAX_VGA_COLS * row)];
      vga_adress[col + (MAX_VGA_COLS * (row - 1))] = character;
    }
  }

  clear_row(MAX_VGA_ROWS - 1);
  update_cursor();
}

void putc(char c) {
  if (c == '\n') {
    newline();
    return;
  }

  vga_adress[cursor_x + (MAX_VGA_COLS * cursor_y)] =
      (struct Char){.character = c, .color = default_color};

  ++cursor_x;

  if (cursor_x >= MAX_VGA_COLS) {
    newline();
  }

  update_cursor();
}

void print_hex(uint64_t value) {
  char hex_str[17];
  const char *hex_digits = "0123456789ABCDEF";
  hex_str[16] = '\0';
  for (int i = 15; i >= 0; --i) {
    hex_str[i] = hex_digits[value & 0xF];
    value >>= 4;
  }
  print_str("0x");
  print_str(hex_str);
}

void print_int(int64_t num) {
  char buffer[10];
  int i = 0;
  bool is_negative = false;

  if (num == 0) {
    print_str("0");
    return;
  }

  if (num < 0) {
    is_negative = true;
    num = -num;
  }

  while (num > 0) {
    buffer[i++] = (num % 10) + '0';
    num /= 10;
  }

  if (is_negative) {
    buffer[i++] = '-';
  }

  buffer[i] = '\0';

  for (int j = 0, k = i - 1; j < k; j++, k--) {
    char temp = buffer[j];
    buffer[j] = buffer[k];
    buffer[k] = temp;
  }

  print_str(buffer);
}

void print_str(const char *str) {
  for (size_t i = 0; str[i] != '\0'; ++i) {
    putc(str[i]);
  }
}
