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
  uint8_t *framebuffer = nullptr;    // Front buffer (visible)
  uint8_t *back_buffer = nullptr;      // Back buffer (rendering)
  uint8_t *saved_buffer = nullptr;     // Backup for alternate screen
  bool double_buffering_enabled = false;
  uint64_t m_last_flush_tick = 0;
  
  // Command Queue for batched rendering (TODO: implement)
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
  uint32_t current_fg_rgb = 0xAAAAAA;
  uint32_t current_bg_rgb = 0x000000;
  bool use_rgb_color = false;

  // Dirty tiles tracking for efficient updates
  static constexpr uint32_t TILE_SIZE = 64;
  uint32_t m_tiles_x = 0;
  uint32_t m_tiles_y = 0;
  uint8_t* m_dirty_tiles = nullptr; // Bitset for dirty tiles

  DisplayFramebuffer();
  void initialize_framebuffer();
  void select_best_font();
  void render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color,
                   uint32_t bg_color);
  uint32_t color_to_pixel(Color c) const;
  void scroll();
  void draw_cursor();
  void erase_cursor();
  
  // Double buffering functions
  void allocate_back_buffer();
  void free_back_buffer();
  void allocate_dirty_tiles();
  void free_dirty_tiles();
  void swap_buffers();
  void wait_vblank();
  void mark_dirty(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
  void update_dirty_rectangles();
  uint8_t* get_render_buffer();

public:
  static DisplayFramebuffer &the() {
    static DisplayFramebuffer drv;
    return drv;
  }

  // Test method for VESA rendering verification
  void test_render();

  /// Flush pending updates to screen
  void flush() override;

  /// Advance to next frame (with double buffering)
  void next_frame();

  /// Finalize initialization (allocate back buffer after system is stable)
  void finalize_initialization();

  void save_screen() override;
  void restore_screen() override;

  void put_char(char c) override;
  void put_codepoint(uint32_t codepoint) override;
  void write(const char *str) override;
  void write_ansi(const char *str) override;
  void write_ansi_n(const char *str, size_t size) override;
  void clear() override;
  void clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
  void copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) override;
  void set_color(Color fg, Color bg) override;
  void set_colors_rgb(uint32_t fg, uint32_t bg) override;
  
  void set_cursor_pos(uint32_t x, uint32_t y) override;
  uint32_t get_cursor_x() const override { return cursor_x; }
  uint32_t get_cursor_y() const override { return cursor_y; }
  void show_cursor(bool visible) override;

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