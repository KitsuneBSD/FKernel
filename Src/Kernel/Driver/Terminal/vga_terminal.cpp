#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace terminal {

static VGATerminal* s_vga_tty = nullptr;

fk::core::Result<void, fk::core::Error> VGATerminal::attach_input(InputDevice* device) {
    [[maybe_unused]] auto* dev = device;
    return {}; // VGA terminal uses PS/2 keyboard by default
}

fk::core::Result<void, fk::core::Error> VGATerminal::attach_output(OutputDevice* device) {
    [[maybe_unused]] auto* dev = device;
    return {}; // VGA terminal uses VGA adapter by default
}

TerminalCapabilities VGATerminal::capabilities() const {
    TerminalCapabilities caps;
    caps.supports_color = true;
    caps.supports_raw_mode = true;
    caps.supports_canonical_mode = true;
    caps.max_rows = m_rows;
    caps.max_cols = m_cols;
    return caps;
}

fk::core::Result<void, fk::core::Error> VGATerminal::set_size(uint16_t rows, uint16_t cols) {
    m_rows = rows;
    m_cols = cols;
    return {};
}

void VGATerminal::get_size(uint16_t& rows, uint16_t& cols) const {
    rows = m_rows;
    cols = m_cols;
}

const char* VGATerminal::type_name() const {
    return "VGA";
}

void VGATerminal::set_active(VGATerminal* terminal) {
    s_vga_tty = terminal;
}

VGATerminal& VGATerminal::the() {
    if (s_vga_tty) return *s_vga_tty;
    
    // Fallback inicial seguro
    static VGATerminal fallback(0);
    return fallback;
}

void VGATerminal::on_char(char c) {
    fk::synchronization::ScopedLock lock(m_lock);
    bool is_active = (TerminalManager::the().active_terminal() == this);

    // 1. Trata Backspace
    if (c == '\b') {
        if (!m_raw_mode) {
            if (m_line_chars > 0) {
                m_input_queue.remove_last();
                m_line_chars--;
                if (m_echo_enabled && is_active) {
                    vga::the().put_char('\b');
                    // Don't flush on every keystroke - let dirty rectangles handle it
                    // Display::the().flush();
                }
            }
        } else {
            // Em modo RAW, apenas enviamos o caractere \b para a fila
            m_input_queue.enqueue(c);
        }
        return;
    }

    // 2. Trata Newline
    if (c == '\n' || c == '\r') {
        c = '\n'; // Normaliza para \n
        m_line_chars = 0;
        if (m_echo_enabled && !m_raw_mode && is_active) {
            vga::the().put_char('\n');
            // Flush on newlines for better responsiveness
            Display::the().flush();
        }
        m_input_queue.enqueue(c);
        return;
    }

    // 3. Caracteres Normais
    if (m_echo_enabled && !m_raw_mode && is_active) {
        vga::the().put_char(c);
        // Flush immediately for responsive keyboard echo with double buffering
        Display::the().flush();
    }

    if (!m_raw_mode) {
        m_line_chars++;
    }
    
    m_input_queue.enqueue(c);
}

void VGATerminal::clear() {
    vga::the().clear();
}

VGATerminal::VGATerminal(int index) : m_index(index), m_ansi_parser(*this) {
    char name_buf[16];
    if (index == -1) {
        set_name("tty");
    } else {
        snprintf(name_buf, sizeof(name_buf), "tty%d", index);
        set_name(name_buf);
    }
    
    if (index == 0) s_vga_tty = this;

    m_rows = static_cast<uint16_t>(Display::the().get_height());
    m_cols = static_cast<uint16_t>(Display::the().get_width());
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buffer) {
    if (size == 0) return 0;
    if (!buffer) return fk::core::Error::InvalidParameter;

    if (m_raw_mode) {
        size_t read_count = 0;
        while (read_count < size) {
            if (!m_input_queue.is_empty()) {
                buffer[read_count++] = static_cast<uint8_t>(m_input_queue.dequeue());
                if (read_count > 0) return read_count;
            } else {
                if (read_count > 0) return read_count;
                SchedulerManager::the().yield();
            }
        }
        return read_count;
    }

    // Canonical mode: wait for a full line (ending in \n)
    while (true) {
        // Check if we have a newline in the queue
        bool has_newline = false;
        size_t newline_pos = 0;
        for (size_t i = 0; i < m_input_queue.size(); ++i) {
            if (m_input_queue[i] == '\n') {
                has_newline = true;
                newline_pos = i;
                break;
            }
        }

        if (has_newline) {
            size_t to_copy = (newline_pos + 1 < size) ? (newline_pos + 1) : size;
            for (size_t i = 0; i < to_copy; ++i) {
                buffer[i] = static_cast<uint8_t>(m_input_queue.dequeue());
            }
            return to_copy;
        }
        
        SchedulerManager::the().yield();
    }
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buffer) {
    if (!buffer) return fk::core::Error::InvalidParameter;
    
fk::synchronization::ScopedLock lock(m_lock);
    // Only active terminal should output to screen
    if (TerminalManager::the().active_terminal() == this) {
        m_ansi_parser.process_data(reinterpret_cast<const char*>(buffer), size);
        Display::the().flush();
    }
    
    return size;
}

struct winsize {
    uint16_t ws_row;
    uint16_t ws_col;
    uint16_t ws_xpixel;
    uint16_t ws_ypixel;
};

// termios constants (simplified)
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TIOCGWINSZ 0x5413

struct termios {
	uint32_t c_iflag;
	uint32_t c_oflag;
	uint32_t c_cflag;
	uint32_t c_lflag;
	uint8_t c_line;
	uint8_t c_cc[32];
	uint32_t c_ispeed;
	uint32_t c_ospeed;
};

#define ICANON 0000002
#define ECHO   0000010

fk::core::Result<int, fk::core::Error> terminal::VGATerminal::ioctl(uint64_t request, uint64_t arg) {
    fk::synchronization::ScopedLock lock(m_lock);
    switch (request) {
        case TIOCGWINSZ: {
            auto* ws = reinterpret_cast<winsize*>(arg);
            if (!ws) return fk::core::Error::InvalidParameter;
            ws->ws_row = m_rows;
            ws->ws_col = m_cols;
            return 0;
        }
        case TCGETS: {
            auto* t = reinterpret_cast<termios*>(arg);
            if (!t) return fk::core::Error::InvalidParameter;
            t->c_lflag = (m_raw_mode ? 0 : ICANON) | (m_echo_enabled ? ECHO : 0);
            return 0;
        }
        case TCSETS: {
            auto* t = reinterpret_cast<termios*>(arg);
            if (!t) return fk::core::Error::InvalidParameter;
            m_raw_mode = !(t->c_lflag & ICANON);
            m_echo_enabled = (t->c_lflag & ECHO);
            return 0;
        }
    }
    return fk::core::Error::NotImplemented;
}

// --- AnsiDelegate Implementation ---

void terminal::VGATerminal::put_char(char c) {
    vga::the().put_char(c);
    // Let the write() method handle flushing - don't flush here
    // Display::the().flush();
}

void terminal::VGATerminal::move_cursor(uint16_t row, uint16_t col) {
    // ANSI uses 1-based coordinates
    vga::the().set_cursor_pos(col - 1, row - 1);
}

void terminal::VGATerminal::move_cursor_up(uint16_t rows) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    if (y >= rows) y -= rows; else y = 0;
    vga::the().set_cursor_pos(x, y);
}

void terminal::VGATerminal::move_cursor_down(uint16_t rows) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    y += rows;
    if (y >= m_rows) y = m_rows - 1;
    vga::the().set_cursor_pos(x, y);
}

void terminal::VGATerminal::move_cursor_forward(uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    x += cols;
    if (x >= m_cols) x = m_cols - 1;
    vga::the().set_cursor_pos(x, y);
}

void terminal::VGATerminal::move_cursor_back(uint16_t cols) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    if (x >= cols) x -= cols; else x = 0;
    vga::the().set_cursor_pos(x, y);
}

void terminal::VGATerminal::set_colors(uint8_t fg, uint8_t bg) {
    vga::the().set_color(static_cast<Color>(fg), static_cast<Color>(bg));
}

void terminal::VGATerminal::clear_screen(uint8_t mode) {
    uint32_t y = vga::the().get_cursor_y();

    switch (mode) {
        case 0: // Clear from cursor to end of screen
            clear_line(0); // Clear rest of current line
            if (y + 1 < m_rows) {
                Display::the().clear_rect(0, (y + 1) * Display::the().get_height() / m_rows, 
                                         Display::the().get_width(), 
                                         Display::the().get_height() - (y + 1) * Display::the().get_height() / m_rows);
            }
            break;
        case 1: // Clear from beginning of screen to cursor
            if (y > 0) {
                Display::the().clear_rect(0, 0, Display::the().get_width(), 
                                         y * Display::the().get_height() / m_rows);
            }
            clear_line(1); // Clear beginning of current line
            break;
        case 2: // Clear entire screen
        case 3: // Clear entire screen including scrollback
            vga::the().clear();
            vga::the().set_cursor_pos(0, 0);
            break;
    }
}

void terminal::VGATerminal::clear_line(uint8_t mode) {
    uint32_t current_x = Display::the().get_cursor_x();
    uint32_t current_y = Display::the().get_cursor_y();
    
    // Save current cursor position
    uint32_t saved_x = current_x;
    uint32_t saved_y = current_y;
    
    switch (mode) {
        case 0: // Clear from cursor to end of line
            for (uint32_t i = current_x; i < m_cols; i++) {
                Display::the().set_cursor_pos(i, current_y);
                Display::the().put_char(' ');
            }
            break;
        case 1: // Clear from beginning to cursor
            for (uint32_t i = 0; i <= current_x; i++) {
                Display::the().set_cursor_pos(i, current_y);
                Display::the().put_char(' ');
            }
            break;
        case 2: // Clear entire line
            for (uint32_t i = 0; i < m_cols; i++) {
                Display::the().set_cursor_pos(i, current_y);
                Display::the().put_char(' ');
            }
            break;
    }
    
    // Restore cursor position
    Display::the().set_cursor_pos(saved_x, saved_y);
}

void terminal::VGATerminal::set_scroll_region(uint16_t top, uint16_t bottom) {
    // VGA/Display currently doesn't support limited scroll regions easily
    (void)top; (void)bottom;
}

void terminal::VGATerminal::save_cursor() {
    m_saved_cursor_x = static_cast<uint16_t>(vga::the().get_cursor_x());
    m_saved_cursor_y = static_cast<uint16_t>(vga::the().get_cursor_y());
}

void terminal::VGATerminal::restore_cursor() {
    vga::the().set_cursor_pos(m_saved_cursor_x, m_saved_cursor_y);
}

void terminal::VGATerminal::save_screen() {
    Display::the().save_screen();
}

void terminal::VGATerminal::restore_screen() {
    Display::the().restore_screen();
}

void terminal::VGATerminal::show_cursor(bool visible) {
    Display::the().show_cursor(visible);
}

uint32_t terminal::VGATerminal::get_cursor_x() {
    return Display::the().get_cursor_x();
}

uint32_t terminal::VGATerminal::get_cursor_y() {
    return Display::the().get_cursor_y();
}

void terminal::VGATerminal::respond(const char* data) {
    if (!data) return;
    // Inject data into input queue as if typed
    while (*data) {
        m_input_queue.enqueue(*data++);
    }
}

// Additional methods for vi compatibility
void terminal::VGATerminal::insert_chars(uint16_t count) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    uint32_t char_w = Display::the().get_width() / m_cols;
    uint32_t char_h = Display::the().get_height() / m_rows;

    if (x + count < m_cols) {
        Display::the().copy_rect(x * char_w, y * char_h, 
                                (x + count) * char_w, y * char_h, 
                                (m_cols - x - count) * char_w, char_h);
    }
    Display::the().clear_rect(x * char_w, y * char_h, count * char_w, char_h);
}

void terminal::VGATerminal::delete_chars(uint16_t count) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    uint32_t char_w = Display::the().get_width() / m_cols;
    uint32_t char_h = Display::the().get_height() / m_rows;

    if (x + count < m_cols) {
        Display::the().copy_rect((x + count) * char_w, y * char_h, 
                                x * char_w, y * char_h, 
                                (m_cols - x - count) * char_w, char_h);
        Display::the().clear_rect((m_cols - count) * char_w, y * char_h, count * char_w, char_h);
    } else {
        Display::the().clear_rect(x * char_w, y * char_h, (m_cols - x) * char_w, char_h);
    }
}

void terminal::VGATerminal::erase_chars(uint16_t count) {
    uint32_t x = vga::the().get_cursor_x();
    uint32_t y = vga::the().get_cursor_y();
    uint32_t char_w = Display::the().get_width() / m_cols;
    uint32_t char_h = Display::the().get_height() / m_rows;

    uint32_t to_clear = (x + count > m_cols) ? (m_cols - x) : count;
    Display::the().clear_rect(x * char_w, y * char_h, to_clear * char_w, char_h);
}

void terminal::VGATerminal::insert_lines(uint16_t count) {
    uint32_t y = vga::the().get_cursor_y();
    uint32_t char_h = Display::the().get_height() / m_rows;

    if (y + count < m_rows) {
        Display::the().copy_rect(0, y * char_h, 
                                0, (y + count) * char_h, 
                                Display::the().get_width(), (m_rows - y - count) * char_h);
    }
    Display::the().clear_rect(0, y * char_h, Display::the().get_width(), count * char_h);
}

void terminal::VGATerminal::delete_lines(uint16_t count) {
    uint32_t y = vga::the().get_cursor_y();
    uint32_t char_h = Display::the().get_height() / m_rows;

    if (y + count < m_rows) {
        Display::the().copy_rect(0, (y + count) * char_h, 
                                0, y * char_h, 
                                Display::the().get_width(), (m_rows - y - count) * char_h);
        Display::the().clear_rect(0, (m_rows - count) * char_h, Display::the().get_width(), count * char_h);
    } else {
        Display::the().clear_rect(0, y * char_h, Display::the().get_width(), (m_rows - y) * char_h);
    }
}

void terminal::VGATerminal::scroll_up(uint16_t count) {
    uint32_t char_h = Display::the().get_height() / m_rows;
    if (count < m_rows) {
        Display::the().copy_rect(0, count * char_h, 
                                0, 0, 
                                Display::the().get_width(), (m_rows - count) * char_h);
        Display::the().clear_rect(0, (m_rows - count) * char_h, Display::the().get_width(), count * char_h);
    } else {
        vga::the().clear();
    }
}

void terminal::VGATerminal::scroll_down(uint16_t count) {

    uint32_t char_h = Display::the().get_height() / m_rows;

    if (count < m_rows) {

        Display::the().copy_rect(0, 0, 

                                0, count * char_h, 

                                Display::the().get_width(), (m_rows - count) * char_h);

        Display::the().clear_rect(0, 0, Display::the().get_width(), count * char_h);

    } else {

        vga::the().clear();

    }

}



void terminal::VGATerminal::set_line_drawing_mode(bool enabled) {

    m_line_drawing_mode = enabled;

}



} // namespace terminal

} // namespace fkernel
