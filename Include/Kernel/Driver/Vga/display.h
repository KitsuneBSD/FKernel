#pragma once

#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Driver/Vga/font.h>
#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>
#include <LibFK/Synchronization/spinlock.h>

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>
#include <LibFK/Container/Sequence/circular_buffer.h>
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

  /// Clear a specific rectangle on the screen
  virtual void clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

  /// Copy a rectangle of pixels from one location to another (for scrolling)
  virtual void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) = 0;

  /// Set foreground and background colors (4-bit palette)
  virtual void set_color(Color fg, Color bg) = 0;

  /// Set foreground and background colors (24-bit RGB)
  virtual void set_colors_rgb(uint32_t fg, uint32_t bg) = 0;

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

  /// Background flush (called from timer/scheduler), must be non-blocking
  virtual void background_flush() = 0;

  /// Save current screen content to a backup buffer
  virtual void save_screen() = 0;

  /// Restore screen content from backup buffer
  virtual void restore_screen() = 0;

  /**
   * @brief Get the appropriate display backend
   */
  static Display &the();

  /**
   * @brief Manually switch the active display driver
   */
  static void switch_to(Display &driver);
};


