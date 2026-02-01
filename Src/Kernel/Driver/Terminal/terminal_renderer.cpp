#include <Kernel/Driver/Terminal/terminal_renderer.h>

namespace fkernel {
namespace terminal {

TerminalRenderer::TerminalRenderer(uint16_t rows, uint16_t cols) 
    : m_rows(rows), m_cols(cols) {}

void TerminalRenderer::put_char(char c) {
    vga::the().put_char(c);
}

void TerminalRenderer::clear() {
    vga::the().clear();
}

void TerminalRenderer::move_cursor(uint16_t row, uint16_t col) {
    vga::the().set_cursor_pos(col - 1, row - 1);
}

void TerminalRenderer::set_colors(uint8_t fg, uint8_t bg) {
    vga::the().set_color(static_cast<Color>(fg), static_cast<Color>(bg));
}

void TerminalRenderer::set_colors_rgb(uint32_t fg, uint32_t bg) {
    Display::the().set_colors_rgb(fg, bg);
}

void TerminalRenderer::save_cursor() {
    m_saved_x = static_cast<uint16_t>(vga::the().get_cursor_x());
    m_saved_y = static_cast<uint16_t>(vga::the().get_cursor_y());
}

void TerminalRenderer::restore_cursor() {
    vga::the().set_cursor_pos(m_saved_x, m_saved_y);
}

void TerminalRenderer::clear_screen(uint8_t mode, uint16_t rows) {
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_h = fb.height / rows;

    if (mode == 0) { // Cursor to end
        if (y + 1 < rows) {
            Display::the().clear_rect(0, (y + 1) * char_h, fb.width, fb.height - (y + 1) * char_h);
        }
    } else if (mode == 1) { // Start to cursor
        if (y > 0) {
            Display::the().clear_rect(0, 0, fb.width, y * char_h);
        }
    } else { // Entire screen
        vga::the().clear();
        vga::the().set_cursor_pos(0, 0);
    }
}

void TerminalRenderer::clear_line(uint8_t mode, uint16_t rows, uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_w = fb.width / cols;
    uint32_t char_h = fb.height / rows;

    if (mode == 0) {
        Display::the().clear_rect(x * char_w, y * char_h, fb.width - (x * char_w), char_h);
    } else if (mode == 1) {
        Display::the().clear_rect(0, y * char_h, (x + 1) * char_w, char_h);
    } else {
        Display::the().clear_rect(0, y * char_h, fb.width, char_h);
    }
}

void TerminalRenderer::scroll_up(uint16_t count, uint16_t rows) {
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_h = fb.height / rows;
    if (count < rows) {
        Display::the().copy_rect(0, count * char_h, 0, 0, fb.width, (rows - count) * char_h);
        Display::the().clear_rect(0, (rows - count) * char_h, fb.width, count * char_h);
    } else {
        vga::the().clear();
    }
}

void TerminalRenderer::scroll_down(uint16_t count, uint16_t rows) {
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_h = fb.height / rows;
    if (count < rows) {
        Display::the().copy_rect(0, 0, 0, count * char_h, fb.width, (rows - count) * char_h);
        Display::the().clear_rect(0, 0, fb.width, count * char_h);
    } else {
        vga::the().clear();
    }
}

void TerminalRenderer::insert_chars(uint16_t count, uint16_t rows, uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_w = fb.width / cols;
    uint32_t char_h = fb.height / rows;

    if (x + count < cols) {
        Display::the().copy_rect(x * char_w, y * char_h, (x + count) * char_w, y * char_h, (cols - x - count) * char_w, char_h);
    }
    Display::the().clear_rect(x * char_w, y * char_h, count * char_w, char_h);
}

void TerminalRenderer::delete_chars(uint16_t count, uint16_t rows, uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_w = fb.width / cols;
    uint32_t char_h = fb.height / rows;

    if (x + count < cols) {
        Display::the().copy_rect((x + count) * char_w, y * char_h, x * char_w, y * char_h, (cols - x - count) * char_w, char_h);
        Display::the().clear_rect((cols - count) * char_w, y * char_h, fb.width - (cols - count) * char_w, char_h);
    } else {
        Display::the().clear_rect(x * char_w, y * char_h, fb.width - (x * char_w), char_h);
    }
}

void TerminalRenderer::erase_chars(uint16_t count, uint16_t rows, uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_w = fb.width / cols;
    uint32_t char_h = fb.height / rows;

    if (x + count >= cols) {
        Display::the().clear_rect(x * char_w, y * char_h, fb.width - (x * char_w), char_h);
    } else {
        Display::the().clear_rect(x * char_w, y * char_h, count * char_w, char_h);
    }
}

void TerminalRenderer::insert_lines(uint16_t count, uint16_t rows) {
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_h = fb.height / rows;

    if (y + count < rows) {
        Display::the().copy_rect(0, y * char_h, 0, (y + count) * char_h, fb.width, (rows - y - count) * char_h);
    }
    Display::the().clear_rect(0, y * char_h, fb.width, count * char_h);
}

void TerminalRenderer::delete_lines(uint16_t count, uint16_t rows) {
    uint32_t y = vga::the().get_cursor_y();
    auto fb = Display::the().get_framebuffer_info();
    uint32_t char_h = fb.height / rows;

    if (y + count < rows) {
        Display::the().copy_rect(0, (y + count) * char_h, 0, y * char_h, fb.width, (rows - y - count) * char_h);
        Display::the().clear_rect(0, (rows - count) * char_h, fb.width, count * char_h);
    } else {
        Display::the().clear_rect(0, y * char_h, fb.width, (rows - y) * char_h);
    }
}

uint32_t TerminalRenderer::cursor_x() const { return vga::the().get_cursor_x(); }
uint32_t TerminalRenderer::cursor_y() const { return vga::the().get_cursor_y(); }

} // namespace terminal
} // namespace fkernel
