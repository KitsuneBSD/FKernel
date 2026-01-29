#pragma once

#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Vga/font.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Vga/Types/color.h>
#include <Kernel/Driver/Vga/Types/framebuffer_info.h>
#include <Kernel/Driver/Vga/Types/render_command.h>
#include <Kernel/Driver/Vga/display.h>
#include <LibFK/Container/circular_buffer.h>

class DisplayFramebuffer : public Display {
 private:
  uint8_t *framebuffer = nullptr;
  
  // Command Queue for batched rendering
  static constexpr size_t QUEUE_SIZE =
      128; 
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
  void draw_cursor();
  void erase_cursor();

public:
  static DisplayFramebuffer &the() {
    static DisplayFramebuffer drv;
    return drv;
  }

  // Test method for VESA rendering verification
  void test_render();

  /// No-op in direct rendering mode
  void flush() {}

  /// No-op in direct rendering mode
  void next_frame() {}

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
