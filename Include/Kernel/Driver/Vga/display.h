#pragma once

#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Vga/font.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>

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

  /**
   * @brief Get the appropriate display backend
   *
   * Returns a text-mode driver for BIOS boots,
   * or a framebuffer-based driver for EFI/VESA boots
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
};

#include <LibFK/Container/CircularBuffer.h>

struct RenderCommand {
  enum class Type { PutChar, Scroll, Clear, SetColor, Flush } type;

  union {
    struct {
      uint32_t codepoint;
    } put_char;
    struct {
      Color fg;
      Color bg;
    } set_color;
  } data;
};

class DisplayFramebuffer : public Display {
private:
  uint8_t *framebuffer = nullptr;
  uint8_t *back_buffer = nullptr;

  // Command Queue for batched rendering
  static constexpr size_t QUEUE_SIZE =
      128; // Increased from 10 to a power of 2 for better performance
  fk::containers::CircularBuffer<RenderCommand, QUEUE_SIZE> m_command_queue;

  uint32_t fb_width = 0;
  uint32_t fb_height = 0;
  uint32_t fb_pitch = 0;
  uint16_t fb_bpp = 0; // Bits per pixel

  uint32_t cursor_x = 0;
  uint32_t cursor_y = 0;

  Vga::Font m_current_font;

  Color current_fg = Color::LightGray;
  Color current_bg = Color::Black;

  DisplayFramebuffer();
  void initialize_framebuffer();
  void select_best_font();
  void render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color,
                   uint32_t bg_color);
  uint32_t color_to_pixel(Color c) const;
  void scroll();

public:
  static DisplayFramebuffer &the() {
    static DisplayFramebuffer drv;
    return drv;
  }

  /// Copy back buffer to front buffer
  void flush();

  void put_char(char c) override;
  void put_codepoint(uint32_t codepoint) override;
  void write(const char *str) override;
  void write_ansi(const char *str) override;
  void write_ansi_n(const char *str, size_t size) override;
  void clear() override;
  void set_color(Color fg, Color bg) override;
  uint32_t get_width() const override {
    return fb_width / (m_current_font.width * m_current_font.scale);
  }
  uint32_t get_height() const override {
    return fb_height / (m_current_font.height * m_current_font.scale);
  }

  fk::core::Result<void, fk::core::Error> set_vesa_mode(uint16_t mode) override;
  FramebufferInfo get_framebuffer_info() const override {
    return {framebuffer, fb_width, fb_height, fb_pitch, fb_bpp};
  }
  fk::core::Result<void, fk::core::Error>
  set_resolution(uint32_t width, uint32_t height, uint32_t bpp) override;
};
