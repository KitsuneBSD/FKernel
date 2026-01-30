#include <Kernel/Arch/x86_64/Driver/Vga/vesa.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibC/stddef.h>

DisplayFramebuffer::DisplayFramebuffer()
    : cursor_x(0), cursor_y(0), m_current_font(Vga::default_font) {
  initialize_framebuffer();
}

void DisplayFramebuffer::select_best_font() {
  m_current_font = Vga::default_font;

  // A escala de 1x é preferida pelo usuário para maior densidade de informação
  m_current_font.scale = 1;
}

void DisplayFramebuffer::initialize_framebuffer() {
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    framebuffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(fb.addr));
    fb_width = fb.width;
    fb_height = fb.height;
    fb_pitch = fb.pitch;
    fb_bpp = fb.bpp;

    select_best_font();

    // Mapear o framebuffer (MB2) se o MemoryManager estiver pronto
    if (MemoryManager::the().is_initialized()) {
      uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
      size_t fb_size = fb_height * fb_pitch;
      for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
        MemoryManager::the().map_page(addr, addr,
                                      PageFlags::Present | PageFlags::Writable |
                                          PageFlags::WriteThrough);
      }
    }

    // Allocate back buffer for double buffering
    allocate_back_buffer();
  } else if (vesa::VESADriver::the().is_available() &&
             vesa::VESADriver::the().get_framebuffer() != nullptr) {
    framebuffer = vesa::VESADriver::the().get_framebuffer();
    fb_width = vesa::VESADriver::the().get_width();
    fb_height = vesa::VESADriver::the().get_height();
    fb_pitch = vesa::VESADriver::the().get_pitch();
    fb_bpp = vesa::VESADriver::the().get_bpp();

     select_best_font();

    // Mapear o framebuffer (VESA) se o MemoryManager estiver pronto
    if (MemoryManager::the().is_initialized()) {
      uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
      size_t fb_size = fb_height * fb_pitch;
      for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
        MemoryManager::the().map_page(addr, addr,
                                          PageFlags::Present | PageFlags::Writable |
                                              PageFlags::WriteThrough);
      }
    }

    // Defer back buffer allocation to later in initialization
    // allocate_back_buffer(); // Will be called later after system is stable
  } else {
    framebuffer = nullptr;
  }
}

fk::core::Result<void, fk::core::Error>
DisplayFramebuffer::set_vesa_mode(uint16_t mode) {
  // Free existing back buffer before changing mode
  free_back_buffer();
  
  auto res = vesa::VESADriver::the().set_mode(mode);
  if (res.is_ok()) {
    // Initialize framebuffer without double buffering allocation
    // (it will be allocated at the end of initialize_framebuffer)
    if (boot::BootInfo::the().has_framebuffer()) {
      auto fb = boot::BootInfo::the().get_framebuffer_info();
      framebuffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(fb.addr));
      fb_width = fb.width;
      fb_height = fb.height;
      fb_pitch = fb.pitch;
      fb_bpp = fb.bpp;
      select_best_font();
      
      // Mapear o framebuffer (MB2) se o MemoryManager estiver pronto
      if (MemoryManager::the().is_initialized()) {
        uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
        size_t fb_size = fb_height * fb_pitch;
        for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
          MemoryManager::the().map_page(addr, addr,
                                            PageFlags::Present | PageFlags::Writable |
                                                PageFlags::WriteThrough);
        }
      }
    } else if (vesa::VESADriver::the().is_available() &&
               vesa::VESADriver::the().get_framebuffer() != nullptr) {
      framebuffer = vesa::VESADriver::the().get_framebuffer();
      fb_width = vesa::VESADriver::the().get_width();
      fb_height = vesa::VESADriver::the().get_height();
      fb_pitch = vesa::VESADriver::the().get_pitch();
      fb_bpp = vesa::VESADriver::the().get_bpp();
      select_best_font();
      
      // Mapear o framebuffer (VESA) se o MemoryManager estiver pronto
      if (MemoryManager::the().is_initialized()) {
        uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
        size_t fb_size = fb_height * fb_pitch;
        for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
          MemoryManager::the().map_page(addr, addr,
                                            PageFlags::Present | PageFlags::Writable |
                                                PageFlags::WriteThrough);
        }
      }
    }
    
    // Now allocate the back buffer
    allocate_back_buffer();
    clear();
  }
  return res;
}

fk::core::Result<void, fk::core::Error>
DisplayFramebuffer::set_resolution(uint32_t width, uint32_t height,
                                   uint32_t bpp) {
  // Free existing back buffer before changing resolution
  free_back_buffer();
  
  auto res = vesa::VESADriver::the().set_resolution(width, height, bpp);
  if (res.is_ok()) {
    // Re-initialize framebuffer manually to avoid double allocation
    if (vesa::VESADriver::the().is_available() &&
        vesa::VESADriver::the().get_framebuffer() != nullptr) {
      framebuffer = vesa::VESADriver::the().get_framebuffer();
      fb_width = vesa::VESADriver::the().get_width();
      fb_height = vesa::VESADriver::the().get_height();
      fb_pitch = vesa::VESADriver::the().get_pitch();
      fb_bpp = vesa::VESADriver::the().get_bpp();
      select_best_font();
      
      // Re-map framebuffer pages if needed
      if (MemoryManager::the().is_initialized()) {
        uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
        size_t fb_size = fb_height * fb_pitch;
        for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
          MemoryManager::the().map_page(addr, addr,
                                            PageFlags::Present | PageFlags::Writable |
                                                PageFlags::WriteThrough);
        }
      }
    }
    
    // Allocate back buffer after framebuffer is set up
    allocate_back_buffer();
    clear();
  }
  return res;
}

uint32_t DisplayFramebuffer::color_to_pixel(Color c) const {
  uint32_t r = 0, g = 0, b = 0;

  switch (c) {
  case Color::Black:
    r = 0x00;
    g = 0x00;
    b = 0x00;
    break;
  case Color::Blue:
    r = 0x00;
    g = 0x00;
    b = 0xAA;
    break;
  case Color::Green:
    r = 0x00;
    g = 0xAA;
    b = 0x00;
    break;
  case Color::Cyan:
    r = 0x00;
    g = 0xAA;
    b = 0xAA;
    break;
  case Color::Red:
    r = 0xAA;
    g = 0x00;
    b = 0x00;
    break;
  case Color::Magenta:
    r = 0xAA;
    g = 0x00;
    b = 0xAA;
    break;
  case Color::Brown:
    r = 0xAA;
    g = 0x55;
    b = 0x00;
    break;
  case Color::LightGray:
    r = 0xAA;
    g = 0xAA;
    b = 0xAA;
    break;
  case Color::DarkGray:
    r = 0x55;
    g = 0x55;
    b = 0x55;
    break;
  case Color::LightBlue:
    r = 0x55;
    g = 0x55;
    b = 0xFF;
    break;
  case Color::LightGreen:
    r = 0x55;
    g = 0xFF;
    b = 0x55;
    break;
  case Color::LightCyan:
    r = 0x55;
    g = 0xFF;
    b = 0xFF;
    break;
  case Color::LightRed:
    r = 0xFF;
    g = 0x55;
    b = 0x55;
    break;
  case Color::LightMagenta:
    r = 0xFF;
    g = 0x55;
    b = 0xFF;
    break;
  case Color::Yellow:
    r = 0xFF;
    g = 0xFF;
    b = 0x55;
    break;
  case Color::White:
    r = 0xFF;
    g = 0xFF;
    b = 0xFF;
    break;
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

void DisplayFramebuffer::render_char(uint32_t x, uint32_t y, char c,
                                     uint32_t fg_color, uint32_t bg_color) {
  uint8_t *target = get_render_buffer();
  if (!target)
    return;

  // Mark this area as dirty for double buffering
  uint32_t font_w = m_current_font.width * m_current_font.scale;
  uint32_t font_h = m_current_font.height * m_current_font.scale;
  mark_dirty(x, y, font_w, font_h);

  const Vga::Font &font = m_current_font;
  if (!font.data)
    return;

  if (c < font.first_char || c > font.last_char)
    c = '?';

  uint32_t char_index = c - font.first_char;
  const uint8_t *glyph = font.data + (char_index * font.height);
  uint8_t scale = font.scale;

  for (uint32_t row = 0; row < font.height; ++row) {
    uint8_t bits = glyph[row];
    for (uint32_t col = 0; col < font.width; ++col) {
      uint32_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;

      for (uint8_t sy = 0; sy < scale; ++sy) {
        for (uint8_t sx = 0; sx < scale; ++sx) {
          uint32_t px = x + (col * scale) + sx;
          uint32_t py = y + (row * scale) + sy;

          if (px >= fb_width || py >= fb_height)
            continue;

          uint32_t offset = py * fb_pitch + px * (fb_bpp / 8);

          if (fb_bpp == 32) {
            *reinterpret_cast<uint32_t *>(target + offset) = color;
          } else if (fb_bpp == 24) {
            target[offset] = color & 0xFF;
            target[offset + 1] = (color >> 8) & 0xFF;
            target[offset + 2] = (color >> 16) & 0xFF;
          }
        }
      }
    }
  }
}

void DisplayFramebuffer::scroll() {
  uint8_t *target = get_render_buffer();
  if (!target)
    return;

  uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t max_rows = get_height();
  if (cursor_y < max_rows)
    return;

  uint32_t scroll_bytes = font_h * fb_pitch;
  uint32_t total_bytes = fb_height * fb_pitch;

  if (scroll_bytes < total_bytes) {
    memmove(target, target + scroll_bytes, total_bytes - scroll_bytes);
  }

  uint32_t bg_pixel = color_to_pixel(current_bg);
  for (uint32_t y = (max_rows - 1) * font_h; y < fb_height; ++y) {
    for (uint32_t x = 0; x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(target + offset) = bg_pixel;
      } else if (fb_bpp == 24) {
        target[offset] = bg_pixel & 0xFF;
        target[offset + 1] = (bg_pixel >> 8) & 0xFF;
        target[offset + 2] = (bg_pixel >> 16) & 0xFF;
      }
    }
  }

  cursor_y = max_rows - 1;
  
  // Mark entire screen as dirty after scroll
  if (double_buffering_enabled) {
    mark_dirty(0, 0, fb_width, fb_height);
  }
}

void DisplayFramebuffer::put_codepoint(uint32_t codepoint) {
  // For now, render directly - command batching would need
  // deferral and flush mechanism which is complex
  
  fk::synchronization::ScopedLock lock(Display::lock());
  erase_cursor();
  uint32_t font_w = m_current_font.width * m_current_font.scale;
  uint32_t font_h = m_current_font.height * m_current_font.scale;

  if (codepoint == '\n') {
    cursor_x = 0;
    cursor_y++;
    scroll();
    draw_cursor();
    return;
  }

  if (codepoint == '\r') {
    cursor_x = 0;
    draw_cursor();
    return;
  }

  if (codepoint == '\t') {
    uint32_t next_tab = (cursor_x + 8) & ~7;
    while (cursor_x < next_tab && cursor_x < get_width()) {
      render_char(cursor_x * font_w, cursor_y * font_h, ' ',
                  color_to_pixel(current_fg), color_to_pixel(current_bg));
      cursor_x++;
    }
    if (cursor_x >= get_width()) {
      cursor_x = 0;
      cursor_y++;
      scroll();
    }
    draw_cursor();
    return;
  }

  if (codepoint == '\b') {
    if (cursor_x > 0) {
      cursor_x--;
    } else if (cursor_y > 0) {
      cursor_y--;
      cursor_x = get_width() - 1;
    } else {
      draw_cursor();
      return;
    }
    render_char(cursor_x * font_w, cursor_y * font_h, ' ',
                color_to_pixel(current_fg), color_to_pixel(current_bg));
    draw_cursor();
    return;
  }

  if (codepoint == ' ') {
    render_char(cursor_x * font_w, cursor_y * font_h, ' ',
                color_to_pixel(current_fg), color_to_pixel(current_bg));
    cursor_x++;
    if (cursor_x >= get_width()) {
      cursor_x = 0;
      cursor_y++;
      scroll();
    }
    draw_cursor();
    return;
  }

  if (cursor_x >= get_width()) {
    cursor_x = 0;
    cursor_y++;
    scroll();
  }

  if (codepoint < 32 && codepoint != 27) {
    draw_cursor();
    return;
  }

  char c = (codepoint < 128) ? static_cast<char>(codepoint) : '?';
  render_char(cursor_x * font_w, cursor_y * font_h, c,
              color_to_pixel(current_fg), color_to_pixel(current_bg));
  cursor_x++;
  draw_cursor();
}

void DisplayFramebuffer::draw_cursor() {
  uint8_t *target = get_render_buffer();
  if (!target) return;
  uint32_t font_w = m_current_font.width * m_current_font.scale;
  uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t px_start = cursor_x * font_w;
  uint32_t py_start = cursor_y * font_h + (font_h - 2); // Linha na base
  uint32_t color = color_to_pixel(current_fg);

  for (uint32_t y = py_start; y < py_start + 2 && y < fb_height; ++y) {
    for (uint32_t x = px_start; x < px_start + font_w && x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(target + offset) = color;
      }
    }
  }
  
  // Mark cursor area as dirty
  mark_dirty(px_start, py_start, font_w, 2);
}

void DisplayFramebuffer::erase_cursor() {
  uint8_t *target = get_render_buffer();
  if (!target) return;
  uint32_t font_w = m_current_font.width * m_current_font.scale;
  uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t px_start = cursor_x * font_w;
  uint32_t py_start = cursor_y * font_h + (font_h - 2);
  uint32_t color = color_to_pixel(current_bg);

  for (uint32_t y = py_start; y < py_start + 2 && y < fb_height; ++y) {
    for (uint32_t x = px_start; x < px_start + font_w && x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(target + offset) = color;
      }
    }
  }
  
  // Mark cursor area as dirty
  mark_dirty(px_start, py_start, font_w, 2);
}

void DisplayFramebuffer::put_char(char c) {
  put_codepoint(static_cast<uint8_t>(c));
}

void DisplayFramebuffer::test_render() {
  fk::algorithms::klog("DISPLAY", "=== VESA Render Test ===");
  put_char('T');
  put_char('E');
  put_char('S');
  put_char('T');
  flush();
  fk::algorithms::klog("DISPLAY", "Test text rendered");
}

void DisplayFramebuffer::write(const char *str) {
  size_t i = 0;
  while (str[i]) {
    uint32_t codepoint = 0;
    uint8_t c = static_cast<uint8_t>(str[i]);
    if (c <= 0x7F) {
      codepoint = c;
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i + 1]) & 0x3F);
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      codepoint = ((c & 0x0F) << 12) |
                  ((static_cast<uint8_t>(str[i + 1]) & 0x3F) << 6) |
                  (static_cast<uint8_t>(str[i + 2]) & 0x3F);
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      codepoint = ((c & 0x07) << 18) |
                  ((static_cast<uint8_t>(str[i + 1]) & 0x3F) << 12) |
                  ((static_cast<uint8_t>(str[i + 2]) & 0x3F) << 6) |
                  (static_cast<uint8_t>(str[i + 3]) & 0x3F);
      i += 4;
    } else {
      codepoint = '?';
      i += 1;
    }
    put_codepoint(codepoint);
  }
}

void DisplayFramebuffer::clear() {
  clear_rect(0, 0, fb_width, fb_height);
  cursor_x = 0;
  cursor_y = 0;
}

void DisplayFramebuffer::clear_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLock lock(Display::lock());
  erase_cursor();
  uint8_t *target = get_render_buffer();
  if (!target)
    return;
  uint32_t bg_pixel = color_to_pixel(current_bg);
  
  uint32_t end_y = (y + height > fb_height) ? fb_height : y + height;
  uint32_t end_x = (x + width > fb_width) ? fb_width : x + width;

  for (uint32_t py = y; py < end_y; ++py) {
    for (uint32_t px = x; px < end_x; ++px) {
      uint32_t offset = py * fb_pitch + px * (fb_bpp / 8);
      if (fb_bpp == 32) {
        *reinterpret_cast<uint32_t *>(target + offset) = bg_pixel;
      } else if (fb_bpp == 24) {
        target[offset] = bg_pixel & 0xFF;
        target[offset + 1] = (bg_pixel >> 8) & 0xFF;
        target[offset + 2] = (bg_pixel >> 16) & 0xFF;
      }
    }
  }
  
  if (double_buffering_enabled) {
    mark_dirty(x, y, width, height);
  }
}

void DisplayFramebuffer::copy_rect(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height) {
  fk::synchronization::ScopedLock lock(Display::lock());
  uint8_t *target = get_render_buffer();
  if (!target) return;

  uint32_t bpp_bytes = fb_bpp / 8;
  
  // Use memmove for safe overlapping copies
  if (src_x == 0 && dst_x == 0 && width == fb_width) {
      // Optimized for full lines
      memmove(target + dst_y * fb_pitch, target + src_y * fb_pitch, height * fb_pitch);
  } else {
      // Rectangular copy
      for (uint32_t i = 0; i < height; ++i) {
          uint32_t sy = (dst_y < src_y) ? i : (height - 1 - i);
          memmove(target + (dst_y + sy) * fb_pitch + dst_x * bpp_bytes,
                  target + (src_y + sy) * fb_pitch + src_x * bpp_bytes,
                  width * bpp_bytes);
      }
  }

  if (double_buffering_enabled) {
      mark_dirty(dst_x, dst_y, width, height);
  }
}

void DisplayFramebuffer::set_color(Color fg, Color bg) {
  fk::synchronization::ScopedLock lock(Display::lock());
  current_fg = fg;
  current_bg = bg;
}

void DisplayFramebuffer::set_cursor_pos(uint32_t x, uint32_t y) {
    fk::synchronization::ScopedLock lock(Display::lock());
    erase_cursor();
    cursor_x = x;
    cursor_y = y;
    draw_cursor();
}

void DisplayFramebuffer::show_cursor(bool visible) {
    if (visible) draw_cursor();
    else erase_cursor();
}

void DisplayFramebuffer::write_ansi(const char *str) {
  write_ansi_n(str, strlen(str));
}

void DisplayFramebuffer::write_ansi_n(const char *str, size_t size) {
  size_t i = 0;
  while (i < size) {
    if (str[i] == '\033' && (i + 1 < size) && str[i + 1] == '[') {
      i += 2;
      int params[4] = {0, 0, 0, 0};
      int param_count = 0;
      while (i < size && ((str[i] >= '0' && str[i] <= '9') || str[i] == ';')) {
        if (str[i] == ';') {
          if (param_count < 3)
            param_count++;
          i++;
          continue;
        }
        params[param_count] = params[param_count] * 10 + (str[i] - '0');
        i++;
      }
      if (param_count > 0 || params[0] != 0)
        param_count++;
      else if (i < size && str[i] != ';')
        param_count = 1;
      char command = (i < size) ? str[i] : 0;
      if (i < size)
        i++;
      if (command == 'm') {
        Color current_fg_color = current_fg;
        Color current_bg_color = current_bg;
        for (int p = 0; p < (param_count == 0 ? 1 : param_count); ++p) {
          int code = params[p];
          switch (code) {
          case 0:
            current_fg_color = Color::LightGray;
            current_bg_color = Color::Black;
            break;
          case 1:
            if (current_fg_color < Color::DarkGray)
              current_fg_color =
                  static_cast<Color>(static_cast<int>(current_fg_color) + 8);
            break;
          case 30:
            current_fg_color = Color::Black;
            break;
          case 31:
            current_fg_color = Color::Red;
            break;
          case 32:
            current_fg_color = Color::Green;
            break;
          case 33:
            current_fg_color = Color::Brown;
            break;
          case 34:
            current_fg_color = Color::Blue;
            break;
          case 35:
            current_fg_color = Color::Magenta;
            break;
          case 36:
            current_fg_color = Color::Cyan;
            break;
          case 37:
            current_fg_color = Color::White;
            break;
          case 40:
            current_bg_color = Color::Black;
            break;
          case 41:
            current_bg_color = Color::Red;
            break;
          case 42:
            current_bg_color = Color::Green;
            break;
          case 43:
            current_bg_color = Color::Brown;
            break;
          case 44:
            current_bg_color = Color::Blue;
            break;
          case 45:
            current_bg_color = Color::Magenta;
            break;
          case 46:
            current_bg_color = Color::Cyan;
            break;
          case 47:
            current_bg_color = Color::White;
            break;
          }
        }
        set_color(current_fg_color, current_bg_color);
      } else if (command == 'J') {
        if (params[0] == 2)
          clear();
      } else if (command == 'H' || command == 'f') {
        cursor_y = (params[0] > 0) ? params[0] - 1 : 0;
        cursor_x = (params[1] > 0) ? params[1] - 1 : 0;
        if (cursor_y >= get_height())
          cursor_y = get_height() - 1;
        if (cursor_x >= get_width())
          cursor_x = get_width() - 1;
      }
    } else {
      uint32_t codepoint = 0;
      uint8_t c = static_cast<uint8_t>(str[i]);
      if (c <= 0x7F) {
        codepoint = c;
        i += 1;
      } else if ((c & 0xE0) == 0xC0 && i + 1 < size) {
        codepoint =
            ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i + 1]) & 0x3F);
        i += 2;
      } else if ((c & 0xF0) == 0xE0 && i + 2 < size) {
        codepoint = ((c & 0x0F) << 12) |
                    ((static_cast<uint8_t>(str[i + 1]) & 0x3F) << 6) |
                    (static_cast<uint8_t>(str[i + 2]) & 0x3F);
        i += 3;
      } else if ((c & 0xF8) == 0xF0 && i + 3 < size) {
        codepoint = ((c & 0x07) << 18) |
                    ((static_cast<uint8_t>(str[i + 1]) & 0x3F) << 12) |
                    ((static_cast<uint8_t>(str[i + 2]) & 0x3F) << 6) |
                    (static_cast<uint8_t>(str[i + 3]) & 0x3F);
        i += 4;
      } else {
        codepoint = '?';
        i += 1;
      }
      put_codepoint(codepoint);
    }
  }
}

// Double buffering implementation
void DisplayFramebuffer::allocate_back_buffer() {
  // Only allocate if we don't already have a back buffer
  if (back_buffer) {
    return; // Already allocated
  }
  
  if (!framebuffer || fb_width == 0 || fb_height == 0) {
    double_buffering_enabled = false;
    return;
  }

  size_t buffer_size = fb_height * fb_pitch;
  
  // Allocate from kernel memory manager
  back_buffer = static_cast<uint8_t*>(MemoryManager::the().allocate(buffer_size));
  
  if (back_buffer) {
    double_buffering_enabled = true;
    fk::algorithms::klog("DISPLAY", "Double buffering enabled: %zu bytes", buffer_size);
    
    // SAFELY initialize back buffer with zeros first
    memset(back_buffer, 0, buffer_size);
    
    // Only copy from framebuffer if it's safe to access
    // Check if MemoryManager is initialized and framebuffer is mapped
    if (MemoryManager::the().is_initialized() && framebuffer) {
      // Try to validate framebuffer accessibility with a small test read first
      volatile uint8_t test_byte = *framebuffer;
      (void)test_byte; // Suppress unused variable warning
      
      // If test read succeeded, copy framebuffer content
      memcpy(back_buffer, framebuffer, buffer_size);
    } else {
      fk::algorithms::kwarn("DISPLAY", "Framebuffer not accessible, using blank back buffer");
    }
  } else {
    double_buffering_enabled = false;
    back_buffer = nullptr;
    fk::algorithms::kwarn("DISPLAY", "Failed to allocate back buffer, using direct rendering");
  }
}

void DisplayFramebuffer::free_back_buffer() {
  if (back_buffer) {
    MemoryManager::the().free(back_buffer);
    back_buffer = nullptr;
    double_buffering_enabled = false;
    fk::algorithms::klog("DISPLAY", "Back buffer freed");
  }
}

void DisplayFramebuffer::wait_vblank() {
  // Skip VSync for immediate keyboard echo responsiveness
  // VSync causes animation lag that breaks typing experience
  // Direct rendering is better for interactive use
}

void DisplayFramebuffer::swap_buffers() {
  if (!double_buffering_enabled || !back_buffer) {
    return;
  }

  // Skip VSync for immediate response - prioritize keyboard echo over tear-free
  // Interactive use benefits more from responsiveness than perfect vsync
  
  // Enhanced safety check: ensure all buffers are valid and accessible
  if (!framebuffer || fb_height == 0 || fb_pitch == 0) {
    return;
  }
  
  // Validate framebuffer accessibility before copying
  volatile uint8_t test_byte = *framebuffer;
  (void)test_byte; // Suppress unused variable warning
  test_byte = *back_buffer; // Test back buffer accessibility too
  (void)test_byte;
  
  // Copy back buffer to front buffer immediately
  size_t buffer_size = fb_height * fb_pitch;
  memcpy(framebuffer, back_buffer, buffer_size);
}

void DisplayFramebuffer::mark_dirty(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  if (!m_dirty_rect.dirty) {
    m_dirty_rect.x = x;
    m_dirty_rect.y = y;
    m_dirty_rect.width = width;
    m_dirty_rect.height = height;
    m_dirty_rect.dirty = true;
  } else {
    // Expand rectangle to include new area
    uint32_t right1 = m_dirty_rect.x + m_dirty_rect.width;
    uint32_t right2 = x + width;
    uint32_t bottom1 = m_dirty_rect.y + m_dirty_rect.height;
    uint32_t bottom2 = y + height;
    
    m_dirty_rect.x = (x < m_dirty_rect.x) ? x : m_dirty_rect.x;
    m_dirty_rect.y = (y < m_dirty_rect.y) ? y : m_dirty_rect.y;
    m_dirty_rect.width = ((right2 > right1) ? right2 : right1) - m_dirty_rect.x;
    m_dirty_rect.height = ((bottom2 > bottom1) ? bottom2 : bottom1) - m_dirty_rect.y;
  }
}

void DisplayFramebuffer::update_dirty_rectangles() {
  if (!m_dirty_rect.dirty || !double_buffering_enabled) {
    return;
  }

  // For keyboard echo, use fast full buffer copy to avoid complexity
  // Single character updates benefit more from simplicity than optimization
  if (m_dirty_rect.width < 100 && m_dirty_rect.height < 100) {
    // Small area (typing) - do immediate full swap for simplicity
    size_t buffer_size = fb_height * fb_pitch;
    memcpy(framebuffer, back_buffer, buffer_size);
  } else {
    // Large area - use dirty rectangle optimization
    uint8_t *src = back_buffer + (m_dirty_rect.y * fb_pitch) + (m_dirty_rect.x * (fb_bpp / 8));
    uint8_t *dst = framebuffer + (m_dirty_rect.y * fb_pitch) + (m_dirty_rect.x * (fb_bpp / 8));
    
    for (uint32_t y = 0; y < m_dirty_rect.height; ++y) {
      memcpy(dst, src, m_dirty_rect.width * (fb_bpp / 8));
      src += fb_pitch;
      dst += fb_pitch;
    }
  }

  m_dirty_rect.dirty = false;
}

uint8_t* DisplayFramebuffer::get_render_buffer() {
  if (double_buffering_enabled && back_buffer) {
    return back_buffer;
  }
  
  // If double buffering is disabled, ensure framebuffer is accessible
  if (!framebuffer) {
    return nullptr; // Prevent null pointer access
  }
  
  return framebuffer;
}

void DisplayFramebuffer::flush() {
  if (!double_buffering_enabled || !back_buffer) {
    return; // Direct rendering - no flush needed
  }

  // Safety check: ensure display is properly initialized
  if (!framebuffer || fb_width == 0 || fb_height == 0) {
    return;
  }

  // Immediate flush for keyboard echo, but only when dirty
  // This maintains responsiveness while reducing unnecessary swaps
  if (m_dirty_rect.dirty) {
    swap_buffers();
    m_dirty_rect.dirty = false;
  }
}

void DisplayFramebuffer::next_frame() {
  // Reset dirty tracking for next frame
  m_dirty_rect.dirty = false;
  m_dirty_rect.x = 0;
  m_dirty_rect.y = 0;
  m_dirty_rect.width = 0;
  m_dirty_rect.height = 0;
}

void DisplayFramebuffer::finalize_initialization() {
  // Allocate back buffer now that system is fully initialized and stable
  allocate_back_buffer();
}
