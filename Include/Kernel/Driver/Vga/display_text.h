#pragma once

#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>

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
  void clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
  void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) override;
  void set_color(Color fg, Color bg) override;
  void set_colors_rgb(uint32_t fg, uint32_t bg) override { (void)fg; (void)bg; }

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
  void background_flush() override {}
  void save_screen() override {}
  void restore_screen() override {}
};
