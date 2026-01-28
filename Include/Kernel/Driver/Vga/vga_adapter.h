#pragma once

#include <Kernel/Driver/Vga/display.h>

/**
 * @brief Compatibility wrapper for legacy VGA class
 * 
 * This maintains backward compatibility with existing code that uses the old
 * vga class, while internally using the new display abstraction that supports
 * both BIOS and EFI boot modes.
 */
class vga {
public:
  /**
   * @brief Get the singleton instance
   * 
   * Automatically selects between BIOS text mode and EFI framebuffer
   * based on the boot method.
   */
  static vga &the() {
    static vga instance;
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

private:
  vga() = default;
};
