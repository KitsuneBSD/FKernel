#pragma once
#include <LibC/stddef.h>
#include <LibC/stdint.h>

/* TODO: Use [Multiboot2
 * Framebuffer](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
 * It's the same thing but we follow 100% of specification
 */

constexpr uint16_t VGA_WIDTH = 80;
constexpr uint16_t VGA_HEIGHT = 25;

enum class Color : uint8_t {
  Black = 0,
  Blue,
  Green,
  Cyan,
  Red,
  Magenta,
  Brown,
  LightGray,
  DarkGray,
  LightBlue,
  LightGreen,
  LightCyan,
  LightRed,
  LightMagenta,
  Yellow,
  White
};

constexpr uint8_t vga_entry_color(Color fg, Color bg) {
  return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

constexpr uint16_t vga_entry(char c, uint8_t color) {
  return (static_cast<uint16_t>(color) << 8) | c;
}

class VGA {
private:
  volatile uint16_t *const buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);
  size_t row = 0;
  size_t col = 0;
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  VGA() = default; // construtor privado

  void scroll() {
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

public:
  // Singleton access
  static VGA &instance() {
    static VGA drv;
    return drv;
  }

  void set_color(Color fg, Color bg) { color = vga_entry_color(fg, bg); }

  void put_char(char c) {
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

  void write(const char *str) {
    for (size_t i = 0; str[i]; ++i) {
      put_char(str[i]);
    }
  }

  void clear() {
    for (size_t r = 0; r < VGA_HEIGHT; ++r) {
      for (size_t c = 0; c < VGA_WIDTH; ++c) {
        buffer[r * VGA_WIDTH + c] = vga_entry(' ', color);
      }
    }
    row = 0;
    col = 0;
  }
};
