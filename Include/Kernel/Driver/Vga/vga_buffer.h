#pragma once
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace multiboot2 { struct TagFramebuffer; }

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

class vga {
private:
  // Dual-mode driver: text-mode buffer or framebuffer.
  enum class Mode { Text, Framebuffer } mode = Mode::Text;

  // Text-mode legacy buffer at 0xB8000
  volatile uint16_t *const text_buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);

  // Framebuffer state
  uintptr_t fb_addr = 0;
  uint32_t fb_width = 0;
  uint32_t fb_height = 0;
  uint32_t fb_pitch = 0;
  uint8_t fb_bpp = 0;
  uint8_t fb_type = 0; // as in multiboot tag

  size_t row = 0;
  size_t col = 0;
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  vga() = default; // private constructor

  void scroll();
  void update_cursor();

  // Framebuffer helpers
  void fb_put_char(char c);
  void fb_clear();
  void fb_put_pixel(uint32_t x, uint32_t y, uint32_t rgba);
  uint32_t color_to_rgba(Color fg, Color bg) const noexcept;

public:
  static vga &the() {
    static vga drv;
    return drv;
  }

  // Initialize from a multiboot2 framebuffer tag. Returns true on success.
  bool initialize_framebuffer(const multiboot2::TagFramebuffer *fb_tag) noexcept;

  void set_color(Color fg, Color bg);
  void put_char(char c);
  void write_ansi(const char *str);
  void write(const char *str);
  void clear();
};
