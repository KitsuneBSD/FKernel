#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

TTYDevice::TTYDevice(int index) : m_index(index) {
    char name_buf[16];
    if (index == -1) {
        set_name("tty");
    } else {
        snprintf(name_buf, sizeof(name_buf), "tty%d", index);
        set_name(name_buf);
    }
}

fk::core::Result<size_t, fk::core::Error> TTYDevice::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buffer) {
    if (size == 0) return 0;
    if (!buffer) return fk::core::Error::InvalidParameter;

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

fk::core::Result<size_t, fk::core::Error> TTYDevice::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buffer) {
    if (!buffer) return fk::core::Error::InvalidParameter;

    for (size_t i = 0; i < size; ++i) {
        vga::the().put_char(static_cast<char>(buffer[i]));
    }

    return size;
}

} // namespace fkernel
