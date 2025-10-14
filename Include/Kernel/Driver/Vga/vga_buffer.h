#pragma once
#include <LibC/stddef.h>
#include <LibC/stdint.h>

/* TODO: Use [Multiboot2
 * Framebuffer](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
 * It's the same thing but we follow 100% of specification
 */

constexpr uint16_t VGA_WIDTH = 80;
constexpr uint16_t VGA_HEIGHT = 25;

enum class Color : uint8_t
{
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

constexpr uint8_t vga_entry_color(Color fg, Color bg)
{
  return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

constexpr uint16_t vga_entry(char c, uint8_t color)
{
  return (static_cast<uint16_t>(color) << 8) | c;
}

class vga
{
private:
  volatile uint16_t *const buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);
  size_t row = 0;
  size_t col = 0;
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  vga() = default; // construtor privado

  void scroll();

public:
  static vga &the()
  {
    static vga drv;
    return drv;
  }

  void set_color(Color fg, Color bg);
  void put_char(char c);
  void write_ansi(const char *str);
  void write(const char *str);
  void clear();
};
