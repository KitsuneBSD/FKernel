#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace terminal {

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

VGATerminal::VGATerminal(int index) : m_index(index) {
    char name_buf[16];
    if (index == -1) {
        set_name("tty");
    } else {
        snprintf(name_buf, sizeof(name_buf), "tty%d", index);
        set_name(name_buf);
    }
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buffer) {
    if (size == 0) return 0;
    if (!buffer) return fk::core::Error::InvalidParameter;

    if (m_raw_mode) {
        size_t read = 0;
        while (read < size) {
            if (PS2Keyboard::the().has_key()) {
                buffer[read++] = static_cast<uint8_t>(PS2Keyboard::the().pop_key());
                // Return immediately in raw mode if we have at least one char
                if (read > 0) return read;
            } else {
                if (read > 0) return read;
                SchedulerManager::the().yield();
            }
        }
        return read;
    }

    // Flush any pending data from previous lines
    if (m_read_index < m_line_len) {
        size_t copied = 0;
        while (m_read_index < m_line_len && copied < size) {
            buffer[copied++] = m_line_buffer[m_read_index++];
        }
        if (m_read_index == m_line_len) {
            m_read_index = 0;
            m_line_len = 0;
        }
        return copied;
    }

    // Blocking read loop (canonical mode simulation)
    while (true) {
        if (PS2Keyboard::the().has_key()) {
            char c = PS2Keyboard::the().pop_key();

            if (c == '\b') { // Backspace
                if (m_line_len > 0) {
                    m_line_len--;
                    // Visual backspace
                    vga::the().put_char('\b');
                    vga::the().put_char(' ');
                    vga::the().put_char('\b');
                }
                continue;
            }

            // Echo character
            vga::the().put_char(c);

            // Buffer character
            if (m_line_len < LINE_BUFFER_SIZE) {
                m_line_buffer[m_line_len++] = c;
            }

            // Return on newline
            if (c == '\n') {
                // fk::algorithms::klog("TTY", "Newline detected, returning line of len %zu", m_line_len);
                size_t copied = 0;
                while (m_read_index < m_line_len && copied < size) {
                    buffer[copied++] = m_line_buffer[m_read_index++];
                }
                if (m_read_index == m_line_len) {
                    m_read_index = 0;
                    m_line_len = 0;
                }
                return copied;
            }
        } else {
            // Yield CPU while waiting for input
            SchedulerManager::the().yield();
        }
    }
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

fk::core::Result<int, fk::core::Error> VGATerminal::ioctl(uint64_t request, uint64_t arg) {
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
            t->c_lflag = (m_raw_mode ? 0 : (ICANON | ECHO));
            return 0;
        }
        case TCSETS: {
            auto* t = reinterpret_cast<termios*>(arg);
            if (!t) return fk::core::Error::InvalidParameter;
            m_raw_mode = !(t->c_lflag & ICANON);
            // fk::algorithms::klog("TTY", "Switched to %s mode", m_raw_mode ? "RAW" : "CANONICAL");
            return 0;
        }
    }
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buffer) {
    if (!buffer) return fk::core::Error::InvalidParameter;

    for (size_t i = 0; i < size; ++i) {
        vga::the().put_char(static_cast<char>(buffer[i]));
    }

    return size;
}

} // namespace terminal
} // namespace fkernel