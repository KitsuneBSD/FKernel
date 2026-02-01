#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace terminal {

static VGATerminal* s_vga_tty = nullptr;

VGATerminal::VGATerminal(int index)
    : m_index(index), m_ansi_parser(*this), m_renderer(25, 80) {
    if (index == 0) s_vga_tty = this;
    m_state.rows = static_cast<uint16_t>(Display::the().get_height());
    m_state.cols = static_cast<uint16_t>(Display::the().get_width());
}

VGATerminal& VGATerminal::the() {
    if (s_vga_tty) return *s_vga_tty;
    static VGATerminal fallback(0);
    return fallback;
}

void VGATerminal::on_char(char c) {
    fk::synchronization::ScopedLock lock(m_lock);
    bool is_active = (TerminalManager::the().active_terminal() == this);

    if (c == '\b' && !m_state.raw_mode) {
        if (m_state.line_chars > 0) {
            m_input_queue.remove_last();
            m_state.line_chars--;
            if (m_state.echo_enabled && is_active) {
                m_renderer.put_char('\b');
                Display::the().flush();
            }
        }
        return;
    }

    if (c == '\n' || c == '\r') {
        c = '\n';
        m_state.line_chars = 0;
        if (m_state.echo_enabled && !m_state.raw_mode && is_active) {
            m_renderer.put_char('\n');
            Display::the().flush();
        }
        m_input_queue.enqueue(c);
        return;
    }

    if (!m_state.raw_mode) m_state.line_chars++;
    if (m_state.echo_enabled && !m_state.raw_mode && is_active) {
        m_renderer.put_char(c);
        Display::the().flush();
    }
    m_input_queue.enqueue(c);
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::read(uint64_t, size_t size, uint8_t* buffer) {
    if (size == 0 || !buffer) return fk::core::Error::InvalidParameter;

    if (m_state.raw_mode) {
        if (m_input_queue.is_empty()) {
            SchedulerManager::the().yield();
            return 0;
        }
        buffer[0] = static_cast<uint8_t>(m_input_queue.dequeue());
        return 1;
    }

    // Canonical mode: wait for full line
    while (true) {
        for (size_t i = 0; i < m_input_queue.size(); ++i) {
            if (m_input_queue[i] == '\n') {
                size_t to_copy = (i + 1 < size) ? (i + 1) : size;
                for (size_t j = 0; j < to_copy; ++j) buffer[j] = m_input_queue.dequeue();
                return to_copy;
            }
        }
        SchedulerManager::the().yield();
    }
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::write(uint64_t, size_t size, const uint8_t* buffer) {
    if (!buffer) return fk::core::Error::InvalidParameter;
    fk::synchronization::ScopedLock lock(m_lock);
    if (TerminalManager::the().active_terminal() == this) {
        m_ansi_parser.process_data(reinterpret_cast<const char*>(buffer), size);
        Display::the().flush();
    }
    return size;
}

// Delegation to Renderer
void VGATerminal::put_char(char c) { m_renderer.put_char(c); }
void VGATerminal::clear() { m_renderer.clear(); }
void VGATerminal::move_cursor(uint16_t r, uint16_t c) { m_renderer.move_cursor(r, c); }
void VGATerminal::set_colors(uint8_t f, uint8_t b) { m_renderer.set_colors(f, b); }
void VGATerminal::set_colors_rgb(uint32_t f, uint32_t b) { m_renderer.set_colors_rgb(f, b); }
void VGATerminal::clear_screen(uint8_t m) { m_renderer.clear_screen(m, m_state.rows); }
void VGATerminal::clear_line(uint8_t m) { m_renderer.clear_line(m, m_state.rows, m_state.cols); }
void VGATerminal::save_cursor() { m_renderer.save_cursor(); }
void VGATerminal::restore_cursor() { m_renderer.restore_cursor(); }
void VGATerminal::scroll_up(uint16_t n) { m_renderer.scroll_up(n, m_state.rows); }
void VGATerminal::scroll_down(uint16_t n) { m_renderer.scroll_down(n, m_state.rows); }
void VGATerminal::insert_chars(uint16_t n) { m_renderer.insert_chars(n, m_state.rows, m_state.cols); }
void VGATerminal::delete_chars(uint16_t n) { m_renderer.delete_chars(n, m_state.rows, m_state.cols); }
void VGATerminal::erase_chars(uint16_t n) { m_renderer.erase_chars(n, m_state.rows, m_state.cols); }
void VGATerminal::insert_lines(uint16_t n) { m_renderer.insert_lines(n, m_state.rows); }
void VGATerminal::delete_lines(uint16_t n) { m_renderer.delete_lines(n, m_state.rows); }

// State accessors
uint32_t VGATerminal::get_cursor_x() { return m_renderer.cursor_x(); }
uint32_t VGATerminal::get_cursor_y() { return m_renderer.cursor_y(); }
uint16_t VGATerminal::get_width() { return m_state.cols; }
uint16_t VGATerminal::get_height() { return m_state.rows; }

void VGATerminal::set_active(VGATerminal* terminal) { s_vga_tty = terminal; }
const char* VGATerminal::type_name() const { return "VGA"; }
fk::core::Result<void, fk::core::Error> VGATerminal::attach_input(InputDevice*) { return {}; }
fk::core::Result<void, fk::core::Error> VGATerminal::attach_output(OutputDevice*) { return {}; }

TerminalCapabilities VGATerminal::capabilities() const {
    return { .supports_color = true, .supports_raw_mode = true, .supports_canonical_mode = true, 
             .max_rows = m_state.rows, .max_cols = m_state.cols };
}

fk::core::Result<void, fk::core::Error> VGATerminal::set_size(uint16_t r, uint16_t c) {
    m_state.rows = r; m_state.cols = c; return {};
}

void VGATerminal::get_size(uint16_t& r, uint16_t& c) const {
    r = m_state.rows; c = m_state.cols;
}

void VGATerminal::respond(const char* data) {
    if (!data) return;
    while (*data) m_input_queue.enqueue(*data++);
}

void VGATerminal::move_cursor_up(uint16_t n) { move_cursor(m_renderer.cursor_y() + 1 - n, m_renderer.cursor_x() + 1); }
void VGATerminal::move_cursor_down(uint16_t n) { move_cursor(m_renderer.cursor_y() + 1 + n, m_renderer.cursor_x() + 1); }
void VGATerminal::move_cursor_forward(uint16_t n) { move_cursor(m_renderer.cursor_y() + 1, m_renderer.cursor_x() + 1 + n); }
void VGATerminal::move_cursor_back(uint16_t n) { move_cursor(m_renderer.cursor_y() + 1, m_renderer.cursor_x() + 1 - n); }
void VGATerminal::save_screen() { Display::the().save_screen(); }
void VGATerminal::restore_screen() { Display::the().restore_screen(); }
void VGATerminal::show_cursor(bool v) { Display::the().show_cursor(v); }
void VGATerminal::set_scroll_region(uint16_t, uint16_t) {}
void VGATerminal::set_line_drawing_mode(bool e) { m_state.line_drawing_mode = e; }

fk::core::Result<int, fk::core::Error> VGATerminal::ioctl(uint64_t request, uint64_t arg) {
    fk::synchronization::ScopedLock lock(m_lock);
    if (request == 0x5413 /* TIOCGWINSZ */) {
        struct ws { uint16_t r, c, x, y; } *w = (ws*)arg;
        if (!w) return fk::core::Error::InvalidParameter;
        w->r = m_state.rows; w->c = m_state.cols; return 0;
    }
    return fk::core::Error::NotImplemented;
}

} // namespace terminal
} // namespace fkernel