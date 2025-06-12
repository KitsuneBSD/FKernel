#pragma once

#include <stddef.h>
#include <stdint.h>

namespace vga {

constexpr uintptr_t VGA_MEMORY = 0xB8000;
constexpr size_t VGA_WIDTH = 80;
constexpr size_t VGA_HEIGHT = 25;

enum class Color : uint8_t {
  Black = 0,
  Blue = 1,
  Green = 2,
  Cyan = 3,
  Red = 4,
  Magenta = 5,
  Brown = 6,
  LightGray = 7,
  DarkGray = 8,
  LightBlue = 9,
  LightGreen = 10,
  LightCyan = 11,
  LightRed = 12,
  Pink = 13,
  Yellow = 14,
  White = 15,
};

class Console {
public:
  void clear();
  void set_color(Color fg, Color bg);
  void putchar(char c);
  void write(const char *str);
  void write_hex(uint64_t value, bool prefix = true, bool uppercase = true);
  void write_dec(uint64_t value);

private:
  uint16_t *buffer = reinterpret_cast<uint16_t *>(VGA_MEMORY);
  size_t row = 0;
  size_t column = 0;
  uint8_t color = encode_color(Color::LightGray, Color::Black);

  static constexpr uint8_t encode_color(Color fg, Color bg) {
    return (static_cast<uint8_t>(bg) << 4) | static_cast<uint8_t>(fg);
  }

  uint16_t make_entry(char c) const;
  void putchar_raw(char c, size_t col, size_t row);
  void new_line();
  void scroll();
  void update_cursor();

  static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
  }
};
} // namespace vga

inline vga::Console console;
