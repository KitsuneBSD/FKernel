#pragma once

#include <Kernel/Driver/Vga/display.h>

/**
 * @brief Compatibility wrapper for legacy VGA class
 * 
 * This maintains backward compatibility with existing code that uses the old
 * vga class, while internally using the new display abstraction that supports
 * both BIOS and EFI boot modes.
 */
class VgaAdapter {
public:
  /**
   * @brief Get the singleton instance
   * 
   * Automatically selects between BIOS text mode and EFI framebuffer
   * based on the boot method.
   */
  static VgaAdapter &the() {
    static VgaAdapter instance;
    return instance;
  }

  /// Set text color (delegates to the active display backend)
  void set_color(Color fg, Color bg) {
    Display::the().set_color(fg, bg);
  }

  /// Output a single character
  void put_char(char c) {
    Display::the().put_char(c);
  }

  /// Output a single Unicode codepoint
  void put_codepoint(uint32_t cp) {
    Display::the().put_codepoint(cp);
  }

  /// Write a null-terminated ANSI string
  void write_ansi(const char *str) {
    Display::the().write_ansi(str);
  }

  /// Write a fixed-size buffer with ANSI escape codes
  void write_ansi_n(const char *str, size_t size) {
    Display::the().write_ansi_n(str, size);
  }

  /// Write a null-terminated string
  void write(const char *str) {
    Display::the().write(str);
  }

  /// Clear the display
  void clear() {
    Display::the().clear();
  }

  /// Set cursor position
  void set_cursor_pos(uint32_t x, uint32_t y) {
    Display::the().set_cursor_pos(x, y);
  }

  /// Get cursor X position
  uint32_t get_cursor_x() const {
    return Display::the().get_cursor_x();
  }

  /// Get cursor Y position
  uint32_t get_cursor_y() const {
    return Display::the().get_cursor_y();
  }

  /// Show or hide cursor
  void show_cursor(bool visible) {
    Display::the().show_cursor(visible);
  }

private:
  VgaAdapter() = default;
};
