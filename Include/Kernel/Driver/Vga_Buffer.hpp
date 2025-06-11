#pragma once

// TODO: Need remake to use freestanding stddef
// TODO: Need remake to use freestanding stdint
// TODO: Need separe .hpp and .cpp

#include <cstddef>
#include <cstdint>
namespace vga {

constexpr std::uintptr_t VGA_MEMORY = 0xB8000;
constexpr std::size_t VGA_WIDTH = 80;
constexpr std::size_t VGA_HEIGHT = 25;

enum class Color : std::uint8_t {
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
private:
  uint16_t *buffer = reinterpret_cast<uint16_t *>(VGA_MEMORY);
  size_t row = 0;
  size_t column = 0;
  uint8_t color = encode_color(Color::LightGray, Color::Black);

  static constexpr uint8_t encode_color(Color fg, Color bg) {
    return (static_cast<uint8_t>(bg) << 4) | static_cast<uint8_t>(fg);
  }

  uint16_t make_entry(char c) const {
    return (static_cast<uint16_t>(color) << 8) | c;
  }

  void putchar_raw(char c, size_t col, size_t row) {
    const size_t index = row * VGA_WIDTH + col;
    buffer[index] = make_entry(c);
  }

  void new_line() {
    column = 0;
    if (++row == VGA_HEIGHT) {
      scroll();
      row = VGA_HEIGHT - 1;
    }
  }

  void scroll() {
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
      for (size_t x = 0; x < VGA_WIDTH; ++x) {
        buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
      }
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
      buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ');
    }
  }

  void update_cursor() {
    uint16_t pos = row * VGA_WIDTH + column;
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
  }

  // TODO: Separe OutB in a IO_PORT separed code
  static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
  }

public:
  void clear() {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
      for (size_t x = 0; x < VGA_WIDTH; ++x) {
        putchar_raw(' ', x, y);
      }
    }
    row = column = 0;
    update_cursor();
  }

  void set_color(Color fg, Color bg) { color = encode_color(fg, bg); }

  void putchar(char c) {
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

  void write_hex(uint64_t value, bool prefix = true, bool uppercase = true) {
    if (prefix) {
      write("0");
      write("x");
    }

    if (value == 0) {
      write("0");
      new_line();
      return;
    }

    char buffer[17]; // 16 dígitos + null terminator
    buffer[16] = '\0';
    int pos = 15;

    while (value != 0 && pos >= 0) {
      uint8_t digit = value & 0xF;
      buffer[pos--] = (digit < 10) ? ('0' + digit)
                                   : ((uppercase ? 'A' : 'a') + (digit - 10));
      value >>= 4;
    }

    write(&buffer[pos + 1]);
    new_line();
  }

  void write_dec(uint64_t value) {
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

  void write(const char *str) {
    while (*str)
      putchar(*str++);
  }
};

inline Console console;
}; // namespace vga
