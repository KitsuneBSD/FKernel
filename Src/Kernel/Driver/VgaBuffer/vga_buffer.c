#include "../../../../Include/Kernel/Driver/vga_buffer.h"
#include <stddef.h>

static const int MAX_VGA_COLS = 80;
static const int MAX_VGA_ROWS = 25;

size_t actual_col = 0;
size_t actual_row = 0;
uint8_t default_color = WHITE | BLACK << 4;

struct Char *vga_adress = (struct Char *)0xb8000;

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
}

void newline() {
  actual_col = 0;

  if (actual_row < MAX_VGA_ROWS - 1) {
    actual_row++;
    return;
  }

  for (size_t row = 1; row < MAX_VGA_ROWS; ++row) {
    for (size_t col = 0; col < MAX_VGA_COLS; ++col) {
      struct Char character = vga_adress[col + (MAX_VGA_COLS * row)];
      vga_adress[col + (MAX_VGA_COLS * (row - 1))] = character;
    }
  }

  clear_row(MAX_VGA_COLS - 1);
}

void putc(char c) {
  if (c == '\n') {
    newline();
  }

  if (actual_col > MAX_VGA_COLS) {
    newline();
  }

  vga_adress[actual_col + (MAX_VGA_COLS * actual_row)] =
      (struct Char){.character = c, .color = default_color};

  ++actual_col;
}

void print_str(const char *str) {
  for (size_t i = 0; str[i] != '\n'; ++i) {
    if (str[i] == '\0') {
      return;
    }

    putc(str[i]);
  }
}
