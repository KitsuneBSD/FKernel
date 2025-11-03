#pragma once

#include <LibFK/Types/types.h>

namespace multiboot2 {
struct TagFramebuffer;
}

// VGA text-mode constants
constexpr uint16_t VGA_WIDTH = 80;
constexpr uint16_t VGA_HEIGHT = 25;

/**
 * @brief VGA color enumeration
 */
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

/**
 * @brief Create a VGA color byte from foreground and background colors
 */
constexpr uint8_t vga_entry_color(Color fg, Color bg) {
  return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

/**
 * @brief Create a VGA text-mode entry combining a character and color
 */
constexpr uint16_t vga_entry(char c, uint8_t color) {
  return (static_cast<uint16_t>(color) << 8) | c;
}

/**
 * @brief VGA text and framebuffer driver
 *
 * Supports both legacy text-mode (0xB8000) and modern framebuffer mode.
 * Provides basic text output, cursor control, scrolling, and color management.
 */
class vga {
private:
  /// Dual-mode driver: text-mode buffer or framebuffer
  enum class Mode { Text, Framebuffer } mode = Mode::Text;

  /// Legacy text-mode buffer at 0xB8000
  volatile uint16_t *const text_buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);

  /// Framebuffer state
  uintptr_t fb_addr = 0;
  uint32_t fb_width = 0;
  uint32_t fb_height = 0;
  uint32_t fb_pitch = 0;
  uint8_t fb_bpp = 0;
  uint8_t fb_type = 0; ///< Framebuffer type from multiboot tag

  /// Current cursor position
  size_t row = 0;
  size_t col = 0;

  /// Current text color (foreground/background)
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  vga() = default; ///< Private constructor for singleton

  /// Scroll the text buffer up by one line
  void scroll();

  /// Update the hardware cursor
  void update_cursor();

  // Framebuffer helpers
  void fb_put_char(char c);
  void fb_clear();
  void fb_put_pixel(uint32_t x, uint32_t y, uint32_t rgba);
  uint32_t color_to_rgba(Color fg, Color bg) const noexcept;

public:
  /**
   * @brief Get the singleton instance of the VGA driver
   * @return Reference to the VGA driver
   */
  static vga &the() {
    static vga drv;
    return drv;
  }

  /**
   * @brief Initialize framebuffer mode from a Multiboot2 framebuffer tag
   * @param fb_tag Pointer to a Multiboot2 framebuffer tag
   * @return true on success, false otherwise
   */
  bool
  initialize_framebuffer(const multiboot2::TagFramebuffer *fb_tag) noexcept;

  /**
   * @brief Set the foreground and background colors for text
   * @param fg Foreground color
   * @param bg Background color
   */
  void set_color(Color fg, Color bg);

  /**
   * @brief Output a single character to the screen
   * @param c Character to print
   */
  void put_char(char c);

  /**
   * @brief Write a null-terminated ANSI string to the screen
   * @param str String to write
   */
  void write_ansi(const char *str);

  /**
   * @brief Write a null-terminated string to the screen
   * @param str String to write
   */
  void write(const char *str);

  /**
   * @brief Clear the screen
   */
  void clear();
};
