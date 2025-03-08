#pragma once

#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum Color : uint8_t {
  BLACK = 0,
  BLUE = 1,
  GREEN = 2,
  CYAN = 3,
  RED = 4,
  MAGENTA = 5,
  BROWN = 6,
  LIGHT_GRAY = 7,
  DARK_GRAY = 8,
  LIGHT_BLUE = 9,
  LIGHT_GREEN = 10,
  LIGHT_CYAN = 11,
  LIGHT_RED = 12,
  PINK = 13,
  YELLOW = 14,
  WHITE = 15,
};

struct Char {
  char character;
  enum Color color;
};

void clear_screen();
void putc(char c);
void print_str(const char *str);
void print_set_color(enum Color bg, enum Color fg);
