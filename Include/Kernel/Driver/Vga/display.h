#pragma once

#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Vga/font.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>
#include <LibFK/Synchronization/spinlock.h>

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>
#include <LibFK/Container/circular_buffer.h>
/**
 * @brief Display backend abstraction
 *
 * Supports both:
 * - BIOS text mode (80x25 characters)
 * - EFI/VESA framebuffer (pixel-based)
 */
class Display {
public:
  virtual ~Display() = default;

  /// Global lock for display operations
  static fk::synchronization::Spinlock& lock() {
      static fk::synchronization::Spinlock s_lock;
      return s_lock;
  }

  /// Write a single character to the display
  virtual void put_char(char c) = 0;

  /// Write a single Unicode codepoint
  virtual void put_codepoint(uint32_t codepoint) = 0;

  /// Write a null-terminated string
  virtual void write(const char *str) = 0;

  /// Write a null-terminated string with ANSI escape codes
  virtual void write_ansi(const char *str) = 0;

  /// Write a fixed-size buffer with ANSI escape codes
  virtual void write_ansi_n(const char *str, size_t size) = 0;

  /// Clear the display
  virtual void clear() = 0;

  /// Set foreground and background colors
  virtual void set_color(Color fg, Color bg) = 0;

  /// Move cursor to specific position
  virtual void set_cursor_pos(uint32_t x, uint32_t y) = 0;

  /// Get cursor X position
  virtual uint32_t get_cursor_x() const = 0;

  /// Get cursor Y position
  virtual uint32_t get_cursor_y() const = 0;

  /// Show or hide cursor
  virtual void show_cursor(bool visible) = 0;

  /// Get display width (in characters for text mode, in pixels for EFI)
  virtual uint32_t get_width() const = 0;

  /// Get display height (in characters for text mode, in pixels for EFI)
  virtual uint32_t get_height() const = 0;

  /// Set VESA mode (if supported)
  virtual fk::core::Result<void, fk::core::Error>
  set_vesa_mode(uint16_t mode) = 0;

  /// Get current framebuffer information
  virtual FramebufferInfo get_framebuffer_info() const = 0;

  /// Set resolution (if supported)
  virtual fk::core::Result<void, fk::core::Error>
  set_resolution(uint32_t width, uint32_t height, uint32_t bpp) = 0;

  /// Flush display updates (for double-buffered displays)
  virtual void flush() = 0;

  /**
   * @brief Get the appropriate display backend
   */
  static Display &the();

  /**
   * @brief Manually switch the active display driver
   */
  static void switch_to(Display &driver);
};

/**
 * @brief BIOS text-mode display driver (legacy, 80x25)
 */
class DisplayText : public Display {
private:
  static constexpr uint16_t WIDTH = 80;
  static constexpr uint16_t HEIGHT = 25;

  volatile uint16_t *const buffer =
      reinterpret_cast<volatile uint16_t *>(0xB8000);
  size_t row = 0;
  size_t col = 0;
  uint8_t color = 0x07; // Light gray on black

  DisplayText();
  void scroll();
  void update_cursor();
  void enable_cursor();

public:
  static DisplayText &the() {
    static DisplayText drv;
    return drv;
  }

  void put_char(char c) override;
  void put_codepoint(uint32_t codepoint) override;
  void write(const char *str) override;
  void write_ansi(const char *str) override;
  void write_ansi_n(const char *str, size_t size) override;
  void clear() override;
  void set_color(Color fg, Color bg) override;
  
  void set_cursor_pos(uint32_t x, uint32_t y) override;
  uint32_t get_cursor_x() const override { return col; }
  uint32_t get_cursor_y() const override { return row; }
  void show_cursor(bool visible) override;

  uint32_t get_width() const override { return WIDTH; }
  uint32_t get_height() const override { return HEIGHT; }

  fk::core::Result<void, fk::core::Error> set_vesa_mode(uint16_t) override {
    return fk::core::Error::NotImplemented;
  }
  FramebufferInfo get_framebuffer_info() const override {
    return {(uint8_t *)buffer, WIDTH, HEIGHT, WIDTH * 2, 0};
  }
  fk::core::Result<void, fk::core::Error> set_resolution(uint32_t, uint32_t,
                                                         uint32_t) override {
    return fk::core::Error::NotImplemented;
  }
  void flush() override {
    // Text mode has immediate updates, no flush needed
  }
};
