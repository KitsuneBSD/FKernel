#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Types/types.h>

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Driver/Vga/vesa.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/io.h>

DisplayFramebuffer::DisplayFramebuffer() : m_cursor_x(0), m_cursor_y(0), m_current_font(Vga::default_font) { initialize_framebuffer(); }
void DisplayFramebuffer::select_best_font() { m_current_font = Vga::default_font; m_current_font.scale = 1; }

void DisplayFramebuffer::initialize_framebuffer() {
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    m_framebuffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(fb.addr));
    m_fb_width = fb.width; m_fb_height = fb.height; m_fb_pitch = fb.pitch; m_fb_bpp = fb.bpp;
    select_best_font();
    if (MemoryManager::the().is_initialized()) {
      uintptr_t fb_addr = reinterpret_cast<uintptr_t>(m_framebuffer);
      size_t fb_size = m_fb_height * m_fb_pitch;
      for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) MemoryManager::the().map_page(addr, addr, PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
    }
    allocate_back_buffer(); allocate_dirty_tiles();
  } else if (vesa::VESADriver::the().is_available()) {
    m_framebuffer = vesa::VESADriver::the().get_framebuffer();
    m_fb_width = vesa::VESADriver::the().get_width(); m_fb_height = vesa::VESADriver::the().get_height(); m_fb_pitch = vesa::VESADriver::the().get_pitch(); m_fb_bpp = vesa::VESADriver::the().get_bpp();
    select_best_font();
    if (MemoryManager::the().is_initialized()) {
      uintptr_t fb_addr = reinterpret_cast<uintptr_t>(m_framebuffer);
      size_t fb_size = m_fb_height * m_fb_pitch;
      for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) MemoryManager::the().map_page(addr, addr, PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
    }
    allocate_back_buffer(); allocate_dirty_tiles();
  }
}

fk::core::Result<void, fk::core::Error> DisplayFramebuffer::set_vesa_mode(uint16_t mode) {
  free_back_buffer(); free_dirty_tiles();
  auto res = vesa::VESADriver::the().set_mode(mode);
  if (res.is_ok()) { initialize_framebuffer(); clear(); }
  return res;
}

fk::core::Result<void, fk::core::Error> DisplayFramebuffer::set_resolution(uint32_t width, uint32_t height, uint32_t bpp) {
  free_back_buffer(); free_dirty_tiles();
  auto res = vesa::VESADriver::the().set_resolution(width, height, bpp);
  if (res.is_ok()) { initialize_framebuffer(); clear(); }
  return res;
}

uint32_t DisplayFramebuffer::color_to_pixel(Color c) const {
  uint32_t r = 0, g = 0, b = 0;
  switch (c) {
  case Color::Black: r = 0x00; g = 0x00; b = 0x00; break;
  case Color::Blue: r = 0x00; g = 0x00; b = 0xAA; break;
  case Color::Green: r = 0x00; g = 0xAA; b = 0x00; break;
  case Color::Cyan: r = 0x00; g = 0xAA; b = 0xAA; break;
  case Color::Red: r = 0xAA; g = 0x00; b = 0x00; break;
  case Color::Magenta: r = 0xAA; g = 0x00; b = 0xAA; break;
  case Color::Brown: r = 0xAA; g = 0x55; b = 0x00; break;
  case Color::LightGray: r = 0xAA; g = 0xAA; b = 0xAA; break;
  case Color::DarkGray: r = 0x55; g = 0x55; b = 0x55; break;
  case Color::LightBlue: r = 0x55; g = 0x55; b = 0xFF; break;
  case Color::LightGreen: r = 0x55; g = 0xFF; b = 0x55; break;
  case Color::LightCyan: r = 0x55; g = 0xFF; b = 0xFF; break;
  case Color::LightRed: r = 0xFF; g = 0x55; b = 0x55; break;
  case Color::LightMagenta: r = 0xFF; g = 0x55; b = 0xFF; break;
  case Color::Yellow: r = 0xFF; g = 0xFF; b = 0x55; break;
  case Color::White: r = 0xFF; g = 0xFF; b = 0xFF; break;
  }
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    if (fb.red_mask > 0) {
      uint32_t pixel = 0;
      pixel |= ((r >> (8 - fb.red_mask)) << fb.red_pos);
      pixel |= ((g >> (8 - fb.green_mask)) << fb.green_pos);
      pixel |= ((b >> (8 - fb.blue_mask)) << fb.blue_pos);
      return pixel;
    }
  }
  return (r << 16) | (g << 8) | b;
}

uint32_t rgb_to_pixel_internal(uint32_t rgb) {
    uint8_t r = (rgb >> 16) & 0xFF; uint8_t g = (rgb >> 8) & 0xFF; uint8_t b = rgb & 0xFF;
    if (boot::BootInfo::the().has_framebuffer()) {
        auto fb = boot::BootInfo::the().get_framebuffer_info();
        if (fb.red_mask > 0) {
            uint32_t pixel = 0;
            pixel |= ((r >> (8 - fb.red_mask)) << fb.red_pos);
            pixel |= ((g >> (8 - fb.green_mask)) << fb.green_pos);
            pixel |= ((b >> (8 - fb.blue_mask)) << fb.blue_pos);
            return pixel;
        }
    }
    return rgb;
}

void DisplayFramebuffer::render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t font_w = m_current_font.width * m_current_font.scale; uint32_t font_h = m_current_font.height * m_current_font.scale;
  const Vga::Font &font = m_current_font; if (!font.data) return;
  if (c < font.first_char || c > font.last_char) c = '?';
  uint32_t char_index = c - font.first_char; const uint8_t *glyph = font.data + (char_index * font.height); uint8_t scale = font.scale;
  for (uint32_t row = 0; row < font.height; ++row) {
    uint8_t bits = glyph[row];
    for (uint32_t col = 0; col < font.width; ++col) {
      uint32_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;
      for (uint8_t sy = 0; sy < scale; ++sy) {
        for (uint8_t sx = 0; sx < scale; ++sx) {
          uint32_t px = x + (col * scale) + sx; uint32_t py = y + (row * scale) + sy;
          if (px >= m_fb_width || py >= m_fb_height) continue;
          uint32_t offset = py * m_fb_pitch + px * (m_fb_bpp / 8);
          if (m_fb_bpp == 32) *reinterpret_cast<uint32_t *>(target + offset) = color;
          else if (m_fb_bpp == 24) { target[offset] = color & 0xFF; target[offset + 1] = (color >> 8) & 0xFF; target[offset + 2] = (color >> 16) & 0xFF; }
          else if (m_fb_bpp == 16 || m_fb_bpp == 15) *reinterpret_cast<uint16_t *>(target + offset) = static_cast<uint16_t>(color);
        }
      }
    }
  }
  mark_dirty(x, y, font_w, font_h);
}

void DisplayFramebuffer::scroll() {
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t font_h = m_current_font.height * m_current_font.scale; uint32_t max_rows = get_height();
  if (m_cursor_y < max_rows) return;
  uint32_t scroll_bytes = font_h * m_fb_pitch; uint32_t total_bytes = m_fb_height * m_fb_pitch;
  if (scroll_bytes < total_bytes) fk::memory::move(target, target + scroll_bytes, total_bytes - scroll_bytes);
  uint32_t bg_pixel = color_to_pixel(m_current_bg);
  for (uint32_t y = (max_rows - 1) * font_h; y < m_fb_height; ++y) {
    uint8_t* line = target + (y * m_fb_pitch);
    for (uint32_t x = 0; x < m_fb_width; ++x) {
      if (m_fb_bpp == 32) reinterpret_cast<uint32_t *>(line)[x] = bg_pixel;
      else if (m_fb_bpp == 16 || m_fb_bpp == 15) reinterpret_cast<uint16_t *>(line)[x] = static_cast<uint16_t>(bg_pixel);
      else if (m_fb_bpp == 24) { line[x*3] = bg_pixel&0xFF; line[x*3+1] = (bg_pixel>>8)&0xFF; line[x*3+2] = (bg_pixel>>16)&0xFF; }
    }
  }
  m_cursor_y = max_rows - 1; m_full_redraw_requested = true;
}

void DisplayFramebuffer::put_codepoint(uint32_t codepoint) {
  fk::synchronization::ScopedLockIRQ lock(Display::lock());
  erase_cursor();
  uint32_t font_w = m_current_font.width * m_current_font.scale; uint32_t font_h = m_current_font.height * m_current_font.scale;
  if (codepoint == '\n') { m_cursor_x = 0; m_cursor_y++; scroll(); draw_cursor(); return; }
  if (codepoint == '\r') { m_cursor_x = 0; draw_cursor(); return; }
  uint32_t fg = m_use_rgb_color ? rgb_to_pixel_internal(m_current_fg_rgb) : color_to_pixel(m_current_fg);
  uint32_t bg = m_use_rgb_color ? rgb_to_pixel_internal(m_current_bg_rgb) : color_to_pixel(m_current_bg);
  if (codepoint == '\t') {
    uint32_t next_tab = (m_cursor_x + 8) & ~7;
    while (m_cursor_x < next_tab && m_cursor_x < get_width()) { render_char(m_cursor_x * font_w, m_cursor_y * font_h, ' ', fg, bg); m_cursor_x++; }
    if (m_cursor_x >= get_width()) { m_cursor_x = 0; m_cursor_y++; scroll(); }
    draw_cursor(); return;
  }
  if (codepoint == '\b') {
    if (m_cursor_x > 0) m_cursor_x--; else if (m_cursor_y > 0) { m_cursor_y--; m_cursor_x = get_width() - 1; }
    else { draw_cursor(); return; }
    render_char(m_cursor_x * font_w, m_cursor_y * font_h, ' ', fg, bg); draw_cursor(); return;
  }
  if (codepoint == ' ') { render_char(m_cursor_x * font_w, m_cursor_y * font_h, ' ', fg, bg); m_cursor_x++; if (m_cursor_x >= get_width()) { m_cursor_x = 0; m_cursor_y++; scroll(); } draw_cursor(); return; }
  if (m_cursor_x >= get_width()) { m_cursor_x = 0; m_cursor_y++; scroll(); }
  if (codepoint < 32 && codepoint != 27) { draw_cursor(); return; }
  char c = (codepoint < 128) ? static_cast<char>(codepoint) : '?';
  render_char(m_cursor_x * font_w, m_cursor_y * font_h, c, fg, bg); m_cursor_x++; draw_cursor();
}

void DisplayFramebuffer::draw_cursor() {
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t font_w = m_current_font.width * m_current_font.scale; uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t px_start = m_cursor_x * font_w; uint32_t py_start = m_cursor_y * font_h + (font_h - 2);
  uint32_t color = m_use_rgb_color ? rgb_to_pixel_internal(m_current_fg_rgb) : color_to_pixel(m_current_fg);
  for (uint32_t y = py_start; y < py_start + 2 && y < m_fb_height; ++y) {
    uint8_t* line = target + (y * m_fb_pitch);
    for (uint32_t x = px_start; x < px_start + font_w && x < m_fb_width; ++x) {
      if (m_fb_bpp == 32) reinterpret_cast<uint32_t *>(line)[x] = color;
      else if (m_fb_bpp == 16 || m_fb_bpp == 15) reinterpret_cast<uint16_t *>(line)[x] = static_cast<uint16_t>(color);
      else if (m_fb_bpp == 24) { line[x*3] = color & 0xFF; line[x*3+1] = (color >> 8) & 0xFF; line[x*3+2] = (color >> 16) & 0xFF; }
    }
  }
  mark_dirty(px_start, py_start, font_w, 2);
}

void DisplayFramebuffer::erase_cursor() {
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t font_w = m_current_font.width * m_current_font.scale; uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t px_start = m_cursor_x * font_w; uint32_t py_start = m_cursor_y * font_h + (font_h - 2);
  uint32_t color = m_use_rgb_color ? rgb_to_pixel_internal(m_current_bg_rgb) : color_to_pixel(m_current_bg);
  for (uint32_t y = py_start; y < py_start + 2 && y < m_fb_height; ++y) {
    uint8_t* line = target + (y * m_fb_pitch);
    for (uint32_t x = px_start; x < px_start + font_w && x < m_fb_width; ++x) {
      if (m_fb_bpp == 32) reinterpret_cast<uint32_t *>(line)[x] = color;
      else if (m_fb_bpp == 16 || m_fb_bpp == 15) reinterpret_cast<uint16_t *>(line)[x] = static_cast<uint16_t>(color);
      else if (m_fb_bpp == 24) { line[x*3] = color & 0xFF; line[x*3+1] = (color >> 8) & 0xFF; line[x*3+2] = (color >> 16) & 0xFF; }
    }
  }
  mark_dirty(px_start, py_start, font_w, 2);
}

void DisplayFramebuffer::put_char(char c) { put_codepoint(static_cast<uint8_t>(c)); }
void DisplayFramebuffer::write(const char *str) {
  size_t i = 0;
  while (str[i]) {
    uint32_t cp = 0; uint8_t c = static_cast<uint8_t>(str[i]);
    if (c <= 0x7F) { cp = c; i += 1; }
    else if ((c & 0xE0) == 0xC0) { cp = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F); i += 2; }
    else if ((c & 0xF0) == 0xE0) { cp = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+2]) & 0x3F); i += 3; }
    else if ((c & 0xF8) == 0xF0) { cp = ((c & 0x07) << 18) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) | ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+3]) & 0x3F); i += 4; }
    else { cp = '?'; i += 1; }
    put_codepoint(cp);
  }
}

void DisplayFramebuffer::clear() {
  fk::synchronization::ScopedLockIRQ lock(Display::lock());
  uint32_t bg_pixel = m_use_rgb_color ? rgb_to_pixel_internal(m_current_bg_rgb) : color_to_pixel(m_current_bg);
  
  // Clear entire memory area (including pitch padding)
  if (m_back_buffer) {
      if (bg_pixel == 0) fk::memory::set(m_back_buffer, 0, m_fb_height * m_fb_pitch);
      else {
          for (uint32_t y = 0; y < m_fb_height; ++y) {
              uint8_t* line = m_back_buffer + (y * m_fb_pitch);
              if (m_fb_bpp == 32) for (uint32_t x = 0; x < m_fb_width; ++x) reinterpret_cast<uint32_t*>(line)[x] = bg_pixel;
              else if (m_fb_bpp == 16 || m_fb_bpp == 15) for (uint32_t x = 0; x < m_fb_width; ++x) reinterpret_cast<uint16_t*>(line)[x] = static_cast<uint16_t>(bg_pixel);
              else for (uint32_t x = 0; x < m_fb_width; ++x) { line[x*3] = bg_pixel&0xFF; line[x*3+1] = (bg_pixel>>8)&0xFF; line[x*3+2] = (bg_pixel>>16)&0xFF; }
          }
      }
  }
  
  if (m_framebuffer) {
      if (bg_pixel == 0) fk::memory::set(m_framebuffer, 0, m_fb_height * m_fb_pitch);
      else {
          for (uint32_t y = 0; y < m_fb_height; ++y) {
              uint8_t* line = m_framebuffer + (y * m_fb_pitch);
              if (m_fb_bpp == 32) for (uint32_t x = 0; x < m_fb_width; ++x) reinterpret_cast<uint32_t*>(line)[x] = bg_pixel;
              else if (m_fb_bpp == 16 || m_fb_bpp == 15) for (uint32_t x = 0; x < m_fb_width; ++x) reinterpret_cast<uint16_t*>(line)[x] = static_cast<uint16_t>(bg_pixel);
              else for (uint32_t x = 0; x < m_fb_width; ++x) { line[x*3] = bg_pixel&0xFF; line[x*3+1] = (bg_pixel>>8)&0xFF; line[x*3+2] = (bg_pixel>>16)&0xFF; }
          }
      }
  }

  // Force all tiles to be marked clean since we just synced manually
  if (m_dirty_tiles) fk::memory::set(m_dirty_tiles, 0, (m_tiles_x * m_tiles_y + 7) / 8);
  
  m_cursor_x = 0; m_cursor_y = 0; m_full_redraw_requested = false;
}

void DisplayFramebuffer::clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLockIRQ lock(Display::lock());
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t bg_pixel = m_use_rgb_color ? rgb_to_pixel_internal(m_current_bg_rgb) : color_to_pixel(m_current_bg);
  uint32_t end_y = (y + height > m_fb_height) ? m_fb_height : y + height;
  uint32_t end_x = (x + width > m_fb_width) ? m_fb_width : x + width;
  for (uint32_t py = y; py < end_y; ++py) {
    uint8_t* line = target + (py * m_fb_pitch);
    for (uint32_t px = x; px < end_x; ++px) {
      if (m_fb_bpp == 32) reinterpret_cast<uint32_t *>(line)[px] = bg_pixel;
      else if (m_fb_bpp == 16 || m_fb_bpp == 15) reinterpret_cast<uint16_t *>(line)[px] = static_cast<uint16_t>(bg_pixel);
      else if (m_fb_bpp == 24) { line[px*3] = bg_pixel&0xFF; line[px*3+1] = (bg_pixel>>8)&0xFF; line[px*3+2] = (bg_pixel>>16)&0xFF; }
    }
  }
  mark_dirty(x, y, width, height);
}

void DisplayFramebuffer::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLockIRQ lock(Display::lock());
  uint8_t *target = get_render_buffer(); if (!target) return;
  uint32_t bpp_bytes = m_fb_bpp / 8;
  if (src_x == 0 && dst_x == 0 && width == m_fb_width) fk::memory::move(target + dst_y * m_fb_pitch, target + src_y * m_fb_pitch, height * m_fb_pitch);
  else { for (uint32_t i = 0; i < height; ++i) {
      uint32_t sy = (dst_y < src_y) ? i : (height - 1 - i);
      fk::memory::move(target + (dst_y + sy) * m_fb_pitch + dst_x * bpp_bytes, target + (src_y + sy) * m_fb_pitch + src_x * bpp_bytes, width * bpp_bytes);
  }}
  mark_dirty(dst_x, dst_y, width, height);
}

void DisplayFramebuffer::set_color(Color fg, Color bg) { fk::synchronization::ScopedLockIRQ lock(Display::lock()); m_current_fg = fg; m_current_bg = bg; m_use_rgb_color = false; }
void DisplayFramebuffer::set_colors_rgb(uint32_t fg, uint32_t bg) { fk::synchronization::ScopedLockIRQ lock(Display::lock()); m_current_fg_rgb = fg; m_current_bg_rgb = bg; m_use_rgb_color = true; }
void DisplayFramebuffer::set_cursor_pos(uint32_t x, uint32_t y) { fk::synchronization::ScopedLockIRQ lock(Display::lock()); erase_cursor(); m_cursor_x = x; m_cursor_y = y; draw_cursor(); }
void DisplayFramebuffer::show_cursor(bool visible) { if (visible) draw_cursor(); else erase_cursor(); }
void DisplayFramebuffer::write_ansi(const char *str) { write_ansi_n(str, fk::memory::length(str)); }
void DisplayFramebuffer::write_ansi_n(const char *str, size_t size) {
  for (size_t i = 0; i < size; ++i)
    put_codepoint(static_cast<uint8_t>(str[i]));
}

void DisplayFramebuffer::allocate_back_buffer() {
  if (m_back_buffer) return;
  size_t buffer_size = m_fb_height * m_fb_pitch;
  m_back_buffer = static_cast<uint8_t*>(MemoryManager::the().allocate(buffer_size));
  if (m_back_buffer) { m_double_buffering_enabled = true; fk::memory::set(m_back_buffer, 0, buffer_size); if (m_framebuffer) fk::memory::copy(m_back_buffer, m_framebuffer, buffer_size); }
}
void DisplayFramebuffer::free_back_buffer() { if (m_back_buffer) { MemoryManager::the().free(m_back_buffer); m_back_buffer = nullptr; m_double_buffering_enabled = false; } }
void DisplayFramebuffer::wait_vblank() {
    while (inb(0x3DA) & 8) { arch_cpu_relax(); }
    while (!(inb(0x3DA) & 8)) { arch_cpu_relax(); }
}
void DisplayFramebuffer::swap_buffers() { if (!m_double_buffering_enabled || !m_back_buffer || !m_framebuffer) return; fk::memory::copy(m_framebuffer, m_back_buffer, m_fb_height * m_fb_pitch); }

void DisplayFramebuffer::mark_dirty(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  if (!m_dirty_tiles) return;
  uint32_t start_tile_x = x / TILE_SIZE; uint32_t start_tile_y = y / TILE_SIZE;
  uint32_t end_tile_x = (x + width + TILE_SIZE - 1) / TILE_SIZE; uint32_t end_tile_y = (y + height + TILE_SIZE - 1) / TILE_SIZE;
  if (end_tile_x > m_tiles_x) end_tile_x = m_tiles_x; if (end_tile_y > m_tiles_y) end_tile_y = m_tiles_y;
  for (uint32_t ty = start_tile_y; ty < end_tile_y; ++ty) {
    for (uint32_t tx = start_tile_x; tx < end_tile_x; ++tx) {
      uint32_t tile_idx = ty * m_tiles_x + tx; m_dirty_tiles[tile_idx / 8] |= (1 << (tile_idx % 8));
    }
  }
}

void DisplayFramebuffer::update_dirty_rectangles() {
  if (!m_dirty_tiles || !m_double_buffering_enabled) return;
  if (m_full_redraw_requested) { fk::memory::copy(m_framebuffer, m_back_buffer, m_fb_height * m_fb_pitch); m_full_redraw_requested = false; return; }
  uint32_t bpp_bytes = m_fb_bpp / 8;
  for (uint32_t ty = 0; ty < m_tiles_y; ++ty) {
    for (uint32_t tx = 0; tx < m_tiles_x; ++tx) {
      uint32_t tile_idx = ty * m_tiles_x + tx;
      if (m_dirty_tiles[tile_idx / 8] & (1 << (tile_idx % 8))) {
        uint32_t x = tx * TILE_SIZE; uint32_t y = ty * TILE_SIZE; uint32_t w = TILE_SIZE; uint32_t h = TILE_SIZE;
        if (x + w > m_fb_width) w = m_fb_width - x; if (y + h > m_fb_height) h = m_fb_height - y;
        uint32_t line_size = w * bpp_bytes;
        for (uint32_t i = 0; i < h; ++i) fk::memory::copy(m_framebuffer + (y + i) * m_fb_pitch + x * bpp_bytes, m_back_buffer + (y + i) * m_fb_pitch + x * bpp_bytes, line_size);
        m_dirty_tiles[tile_idx / 8] &= ~(1 << (tile_idx % 8));
      }
    }
  }
}

uint8_t* DisplayFramebuffer::get_render_buffer() { if (m_double_buffering_enabled && m_back_buffer) return m_back_buffer; return m_framebuffer; }
void DisplayFramebuffer::flush() {
  if (!m_double_buffering_enabled || !m_back_buffer) return;
  fk::synchronization::ScopedLockIRQ lock(Display::lock());
  if (m_dirty_tiles)
    update_dirty_rectangles();
  else
    fk::memory::copy(m_framebuffer, m_back_buffer, m_fb_height * m_fb_pitch);
  m_last_flush_tick = TickManager::the().get_ticks();
}
void DisplayFramebuffer::background_flush() {
  if (!m_double_buffering_enabled || !m_back_buffer || !m_dirty_tiles) return;
  uint64_t current_tick = TickManager::the().get_ticks(); uint32_t freq = TickManager::the().get_frequency(); if (freq == 0) freq = 100;
  if (current_tick >= m_last_flush_tick + (freq / 60)) {
    if (Display::lock().try_lock()) { update_dirty_rectangles(); m_last_flush_tick = current_tick; Display::lock().unlock(); }
  }
}
void DisplayFramebuffer::next_frame() {}
void DisplayFramebuffer::allocate_dirty_tiles() {
  if (m_dirty_tiles) return;
  m_tiles_x = (m_fb_width + TILE_SIZE - 1) / TILE_SIZE; m_tiles_y = (m_fb_height + TILE_SIZE - 1) / TILE_SIZE;
  size_t bitset_size = (m_tiles_x * m_tiles_y + 7) / 8;
  m_dirty_tiles = static_cast<uint8_t*>(MemoryManager::the().allocate(bitset_size));
  if (m_dirty_tiles) fk::memory::set(m_dirty_tiles, 0xFF, bitset_size);
}
void DisplayFramebuffer::free_dirty_tiles() { if (m_dirty_tiles) { MemoryManager::the().free(m_dirty_tiles); m_dirty_tiles = nullptr; } }
void DisplayFramebuffer::finalize_initialization() { allocate_back_buffer(); allocate_dirty_tiles(); }
void DisplayFramebuffer::save_screen() { if (!m_back_buffer) return; size_t buffer_size = m_fb_height * m_fb_pitch; if (!m_saved_buffer) m_saved_buffer = static_cast<uint8_t*>(MemoryManager::the().allocate(buffer_size)); if (m_saved_buffer) fk::memory::copy(m_saved_buffer, m_back_buffer, buffer_size); }
void DisplayFramebuffer::restore_screen() { if (!m_back_buffer || !m_saved_buffer) return; size_t buffer_size = m_fb_height * m_fb_pitch; fk::memory::copy(m_back_buffer, m_saved_buffer, buffer_size); m_full_redraw_requested = true; }
void DisplayFramebuffer::test_render() {}