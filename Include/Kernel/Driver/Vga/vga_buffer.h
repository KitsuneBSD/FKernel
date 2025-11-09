#pragma once

#include <LibFK/Types/types.h>

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
 * @brief VGA text-mode driver
 *
 * Provides basic text output, cursor control, scrolling, and color management.
 */
class vga {
private:
  /// Legacy text-mode buffer at 0xB8000
  volatile uint16_t *const text_buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);

  /// Current cursor position
  size_t row = 0;
  size_t col = 0;

  /// Current text color (foreground/background)
  uint8_t color = vga_entry_color(Color::LightGray, Color::Black);

  vga(); ///< Private constructor for singleton

  /// Scroll the text buffer up by one line
  void scroll();

  /// Update the hardware cursor
  void update_cursor();
  /// Enable and configure the hardware cursor
  void enable_cursor();

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