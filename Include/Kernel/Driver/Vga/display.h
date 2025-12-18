#pragma once

#include <Kernel/Boot/boot_info.h>
#include <LibFK/Types/types.h>

/**
 * @brief VGA color enumeration (works for both text and pixel modes)
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
 * @brief Display backend abstraction
 * 
 * Supports both:
 * - BIOS text mode (80x25 characters)
 * - EFI framebuffer (pixel-based)
 */
class display {
public:
  virtual ~display() = default;

  /// Write a single character to the display
  virtual void put_char(char c) = 0;

  /// Write a null-terminated string
  virtual void write(const char *str) = 0;

  /// Write a null-terminated string with ANSI escape codes
  virtual void write_ansi(const char *str) = 0;

  /// Clear the display
  virtual void clear() = 0;

  /// Set foreground and background colors
  virtual void set_color(Color fg, Color bg) = 0;

  /// Get display width (in characters for text mode, in pixels for EFI)
  virtual uint32_t get_width() const = 0;

  /// Get display height (in characters for text mode, in pixels for EFI)
  virtual uint32_t get_height() const = 0;

  /**
   * @brief Get the appropriate display backend
   * 
   * Returns a text-mode driver for BIOS boots,
   * or a framebuffer-based driver for EFI boots
   */
  static display &the();
};

/**
 * @brief BIOS text-mode display driver (legacy, 80x25)
 */
class display_text : public display {
private:
  static constexpr uint16_t WIDTH = 80;
  static constexpr uint16_t HEIGHT = 25;

  volatile uint16_t *const buffer = reinterpret_cast<volatile uint16_t *>(0xB8000);
  size_t row = 0;
  size_t col = 0;
  uint8_t color = 0x07; // Light gray on black

  display_text();
  void scroll();
  void update_cursor();
  void enable_cursor();

public:
  static display_text &the() {
    static display_text drv;
    return drv;
  }

  void put_char(char c) override;
  void write(const char *str) override;
  void write_ansi(const char *str) override;
  void clear() override;
  void set_color(Color fg, Color bg) override;
  uint32_t get_width() const override { return WIDTH; }
  uint32_t get_height() const override { return HEIGHT; }
};

/**
 * @brief EFI framebuffer display driver
 * 
 * Uses the framebuffer provided by the EFI firmware via Multiboot2 tags.
 * Implements basic font rendering for text output.
 */
class display_efi : public display {
private:
  uint8_t *framebuffer = nullptr;
  uint32_t fb_width = 0;
  uint32_t fb_height = 0;
  uint32_t fb_pitch = 0;
  uint16_t fb_bpp = 0; // Bits per pixel
  
  // Text cursor position (in characters, scaled to framebuffer)
  uint32_t cursor_x = 0;
  uint32_t cursor_y = 0;
  
  // Text dimensions (character size)
  static constexpr uint32_t CHAR_WIDTH = 8;
  static constexpr uint32_t CHAR_HEIGHT = 16;
  
  Color current_fg = Color::LightGray;
  Color current_bg = Color::Black;

  display_efi();
  void initialize_framebuffer();
  void render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
  uint32_t color_to_pixel(Color c) const;
  void scroll();

public:
  static display_efi &the() {
    static display_efi drv;
    return drv;
  }

  void put_char(char c) override;
  void write(const char *str) override;
  void write_ansi(const char *str) override;
  void clear() override;
  void set_color(Color fg, Color bg) override;
  uint32_t get_width() const override { return fb_width / CHAR_WIDTH; }
  uint32_t get_height() const override { return fb_height / CHAR_HEIGHT; }
};
