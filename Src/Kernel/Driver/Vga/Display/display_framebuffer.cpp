#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Arch/x86_64/Driver/Vga/vesa.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Text/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

DisplayFramebuffer::DisplayFramebuffer() : cursor_x(0), cursor_y(0), m_current_font(Vga::default_font) {
  initialize_framebuffer();
}

void DisplayFramebuffer::select_best_font() {
    m_current_font = Vga::default_font;
    
    // Se a resolução for 1024x768 ou maior, usamos escala 2x para conforto (16x32px)
    if (fb_width >= 1024 || fb_height >= 768) {
        m_current_font.scale = 2;
        fk::algorithms::klog("DISPLAY", "HiDPI threshold reached, using 2x font scale (%ux%u)", 
                             m_current_font.width * 2, m_current_font.height * 2);
    } else {
        m_current_font.scale = 1;
    }
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

    // Mapear o framebuffer (UEFI/MB2) se o MemoryManager estiver pronto
    if (MemoryManager::the().is_initialized()) {
        uintptr_t fb_addr = reinterpret_cast<uintptr_t>(framebuffer);
        size_t fb_size = fb_height * fb_pitch;
        for (uintptr_t addr = fb_addr; addr < fb_addr + fb_size; addr += 4096) {
            MemoryManager::the().map_page(addr, addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
        }

        // Alocar back buffer em RAM se o Heap estiver pronto
        if (MemoryManager::the().is_heap_initialized()) {
            if (back_buffer) kfree(back_buffer);
            back_buffer = static_cast<uint8_t*>(kmalloc(fb_size));
            fk::algorithms::klog("DISPLAY", "Back buffer allocated at %p (%zu bytes)", back_buffer, fb_size);
        }
    }

    fk::algorithms::klog("DISPLAY", "Using BootInfo Framebuffer: %ux%u %ubpp at %p", 
                         fb_width, fb_height, fb_bpp, framebuffer);
  } else if (vesa::VESADriver::the().is_available() && vesa::VESADriver::the().get_framebuffer() != nullptr) {
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
            MemoryManager::the().map_page(addr, addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
        }

        // Alocar back buffer em RAM
        if (MemoryManager::the().is_heap_initialized()) {
            if (back_buffer) kfree(back_buffer);
            back_buffer = static_cast<uint8_t*>(kmalloc(fb_size));
            fk::algorithms::klog("DISPLAY", "Back buffer (VESA) allocated at %p (%zu bytes)", back_buffer, fb_size);
        }
    }
  } else {
    framebuffer = nullptr;
  }
}

void DisplayFramebuffer::flush() {
    if (!framebuffer || !back_buffer) return;
    memcpy(framebuffer, back_buffer, fb_height * fb_pitch);
}

fk::core::Result<void, fk::core::Error> DisplayFramebuffer::set_vesa_mode(uint16_t mode) {
    auto res = vesa::VESADriver::the().set_mode(mode);
    if (res.is_ok()) {
        initialize_framebuffer(); 
        clear();
        flush();
    }
    return res;
}

fk::core::Result<void, fk::core::Error> DisplayFramebuffer::set_resolution(uint32_t width, uint32_t height, uint32_t bpp) {
    auto res = vesa::VESADriver::the().set_resolution(width, height, bpp);
    if (res.is_ok()) {
        initialize_framebuffer(); 
        clear();
        flush();
    }
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

void DisplayFramebuffer::render_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, 
                               uint32_t bg_color) {
  uint8_t* target = back_buffer ? back_buffer : framebuffer;
  if (!target) return;

  const Vga::Font &font = m_current_font;
  if (!font.data) return;

  if (c < font.first_char || c > font.last_char) c = '?';
  
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

              if (px >= fb_width || py >= fb_height) continue;

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
  uint8_t* target = back_buffer ? back_buffer : framebuffer;
  if (!target) return;

  uint32_t font_h = m_current_font.height * m_current_font.scale;
  uint32_t max_rows = get_height();
  if (cursor_y < max_rows) return;

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
}

void DisplayFramebuffer::put_codepoint(uint32_t codepoint) {
  uint32_t font_w = m_current_font.width * m_current_font.scale;
  uint32_t font_h = m_current_font.height * m_current_font.scale;

  if (codepoint == '\n') {
    cursor_x = 0;
    cursor_y++;
    scroll();
    return;
  }

  if (codepoint == '\r') {
    cursor_x = 0;
    return;
  }

  if (codepoint == '\t') {
    uint32_t next_tab = (cursor_x + 8) & ~7;
    while (cursor_x < next_tab && cursor_x < get_width()) {
        render_char(cursor_x * font_w, cursor_y * font_h, ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
        cursor_x++;
    }
    if (cursor_x >= get_width()) {
        cursor_x = 0;
        cursor_y++;
        scroll();
    }
    return;
  }

  if (codepoint == '\b') {
    if (cursor_x > 0) {
      cursor_x--;
      render_char(cursor_x * font_w, cursor_y * font_h, ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
    }
    return;
  }

  if (codepoint == ' ') {
    render_char(cursor_x * font_w, cursor_y * font_h, ' ', color_to_pixel(current_fg), color_to_pixel(current_bg));
    cursor_x++;
    if (cursor_x >= get_width()) {
      cursor_x = 0;
      cursor_y++;
      scroll();
    }
    return;
  }

  if (cursor_x >= get_width()) {
    cursor_x = 0;
    cursor_y++;
    scroll();
  }

  if (codepoint < 32 && codepoint != 27) return;

  char c = (codepoint < 128) ? static_cast<char>(codepoint) : '?';
  render_char(cursor_x * font_w, cursor_y * font_h, c, color_to_pixel(current_fg), color_to_pixel(current_bg));
  cursor_x++;
}

void DisplayFramebuffer::put_char(char c) {
    put_codepoint(static_cast<uint8_t>(c));
}

void DisplayFramebuffer::write(const char *str) {
  size_t i = 0;
  while (str[i]) {
      uint32_t codepoint = 0;
      uint8_t c = static_cast<uint8_t>(str[i]);
      if (c <= 0x7F) { codepoint = c; i += 1; }
      else if ((c & 0xE0) == 0xC0) { codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F); i += 2; }
      else if ((c & 0xF0) == 0xE0) { codepoint = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+2]) & 0x3F); i += 3; }
      else if ((c & 0xF8) == 0xF0) { codepoint = ((c & 0x07) << 18) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) | ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+3]) & 0x3F); i += 4; }
      else { codepoint = '?'; i += 1; }
      put_codepoint(codepoint);
  }
}

void DisplayFramebuffer::clear() {
  uint8_t* target = back_buffer ? back_buffer : framebuffer;
  if (!target) return;
  uint32_t bg_pixel = color_to_pixel(current_bg);
  for (uint32_t y = 0; y < fb_height; ++y) {
    for (uint32_t x = 0; x < fb_width; ++x) {
      uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
      if (fb_bpp == 32) { *reinterpret_cast<uint32_t *>(target + offset) = bg_pixel; }
      else if (fb_bpp == 24) { target[offset] = bg_pixel & 0xFF; target[offset + 1] = (bg_pixel >> 8) & 0xFF; target[offset + 2] = (bg_pixel >> 16) & 0xFF; }
    }
  }
  cursor_x = 0; cursor_y = 0;
}

void DisplayFramebuffer::set_color(Color fg, Color bg) {
  current_fg = fg; current_bg = bg;
}

void DisplayFramebuffer::write_ansi(const char *str) {
  write_ansi_n(str, strlen(str));
}

void DisplayFramebuffer::write_ansi_n(const char *str, size_t size) {
  size_t i = 0;
  while (i < size) {
    if (str[i] == '\033' && (i + 1 < size) && str[i + 1] == '[') {
      i += 2;
      int params[4] = {0, 0, 0, 0}; int param_count = 0;
      while (i < size && ((str[i] >= '0' && str[i] <= '9') || str[i] == ';')) {
          if (str[i] == ';') { if (param_count < 3) param_count++; i++; continue; }
          params[param_count] = params[param_count] * 10 + (str[i] - '0'); i++;
      }
      if (param_count > 0 || params[0] != 0) param_count++;
      else if (i < size && str[i] != ';' ) param_count = 1;
      char command = (i < size) ? str[i] : 0;
      if (i < size) i++;
      if (command == 'm') {
          Color current_fg_color = current_fg; Color current_bg_color = current_bg;
          for (int p = 0; p < (param_count == 0 ? 1 : param_count); ++p) {
              int code = params[p];
              switch (code) {
              case 0: current_fg_color = Color::LightGray; current_bg_color = Color::Black; break;
              case 1: if (current_fg_color < Color::DarkGray) current_fg_color = static_cast<Color>(static_cast<int>(current_fg_color) + 8); break;
              case 30: current_fg_color = Color::Black; break;
              case 31: current_fg_color = Color::Red; break;
              case 32: current_fg_color = Color::Green; break;
              case 33: current_fg_color = Color::Brown; break;
              case 34: current_fg_color = Color::Blue; break;
              case 35: current_fg_color = Color::Magenta; break;
              case 36: current_fg_color = Color::Cyan; break;
              case 37: current_fg_color = Color::White; break;
              case 40: current_bg_color = Color::Black; break;
              case 41: current_bg_color = Color::Red; break;
              case 42: current_bg_color = Color::Green; break;
              case 43: current_bg_color = Color::Brown; break;
              case 44: current_bg_color = Color::Blue; break;
              case 45: current_bg_color = Color::Magenta; break;
              case 46: current_bg_color = Color::Cyan; break;
              case 47: current_bg_color = Color::White; break;
              }
          }
          set_color(current_fg_color, current_bg_color);
      } else if (command == 'J') { if (params[0] == 2) clear(); }
      else if (command == 'H' || command == 'f') {
          cursor_y = (params[0] > 0) ? params[0] - 1 : 0;
          cursor_x = (params[1] > 0) ? params[1] - 1 : 0;
          if (cursor_y >= get_height()) cursor_y = get_height() - 1;
          if (cursor_x >= get_width()) cursor_x = get_width() - 1;
      }
    } else {
      uint32_t codepoint = 0; uint8_t c = static_cast<uint8_t>(str[i]);
      if (c <= 0x7F) { codepoint = c; i += 1; }
      else if ((c & 0xE0) == 0xC0 && i + 1 < size) { codepoint = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F); i += 2; }
      else if ((c & 0xF0) == 0xE0 && i + 2 < size) { codepoint = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+2]) & 0x3F); i += 3; }
      else if ((c & 0xF8) == 0xF0 && i + 3 < size) { codepoint = ((c & 0x07) << 18) | ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) | ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) | (static_cast<uint8_t>(str[i+3]) & 0x3F); i += 4; }
      else { codepoint = '?'; i += 1; }
      put_codepoint(codepoint);
    }
  }
}