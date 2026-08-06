#include <Kernel/Driver/Terminal/serial_terminal.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace terminal {

SerialTerminal::SerialTerminal(const char*) {
    // Serial port is initialized at boot (Serial::init() in kernel_entry);
    // nothing extra needed here.
}

fk::core::Result<size_t, fk::core::Error>
SerialTerminal::read(uint64_t, size_t size, uint8_t* buffer) {
    if (!buffer || size == 0)
        return fk::core::Error::InvalidParameter;
    size_t n = Serial::read(buffer, size);
    return n;
}

fk::core::Result<size_t, fk::core::Error>
SerialTerminal::write(uint64_t, size_t size, const uint8_t* buffer) {
    if (!buffer || size == 0)
        return fk::core::Error::InvalidParameter;
    Serial::write_buffer(reinterpret_cast<const char*>(buffer), size);
    return size;
}

fk::core::Result<int, fk::core::Error>
SerialTerminal::ioctl(uint64_t request, uint64_t arg) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);

    struct linux_termios {
        unsigned int  c_iflag, c_oflag, c_cflag, c_lflag;
        unsigned char c_line;
        unsigned char c_cc[19];
    };
    static constexpr unsigned int ICANON = 0x2;
    static constexpr unsigned int ECHO   = 0x8;
    static constexpr unsigned int ISIG   = 0x1;

    if (request == 0x5401 /* TCGETS */) {
        struct linux_termios kt{};
        kt.c_cflag = 0xBF; // B9600 | CS8
        if (m_echo_enabled) kt.c_lflag |= ECHO;
        if (!m_raw_mode)    kt.c_lflag |= ICANON;
        if (m_isig_enabled) kt.c_lflag |= ISIG;
        kt.c_cc[4] = 1; // VMIN
        if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &kt, sizeof(kt)).is_error())
            return fk::core::Error::InvalidParameter;
        return 0;
    }
    if (request == 0x5402 /* TCSETS */ || request == 0x5403 /* TCSETSW */ ||
        request == 0x5404 /* TCSETSF */) {
        struct linux_termios kt{};
        if (fkernel::memory::copy_from_user(&kt, reinterpret_cast<const void*>(arg), sizeof(kt)).is_error())
            return fk::core::Error::InvalidParameter;
        m_echo_enabled = (kt.c_lflag & ECHO)   != 0;
        m_raw_mode     = (kt.c_lflag & ICANON) == 0;
        m_isig_enabled = (kt.c_lflag & ISIG)   != 0;
        return 0;
    }
    if (request == 0x5413 /* TIOCGWINSZ */) {
        struct ws { uint16_t r, c, x, y; } w{m_rows, m_cols, 0, 0};
        if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &w, sizeof(w)).is_error())
            return fk::core::Error::InvalidParameter;
        return 0;
    }
    if (request == 0x5414 /* TIOCSWINSZ */) {
        struct ws { uint16_t r, c, x, y; } w{};
        if (fkernel::memory::copy_from_user(&w, reinterpret_cast<const void*>(arg), sizeof(w)).is_error())
            return fk::core::Error::InvalidParameter;
        if (w.r > 0) m_rows = w.r;
        if (w.c > 0) m_cols = w.c;
        return 0;
    }

    return fk::core::Error::NotImplemented;
}

TerminalCapabilities SerialTerminal::capabilities() const {
    return TerminalCapabilities{
        .supports_color         = false,
        .supports_raw_mode      = true,
        .supports_canonical_mode = true,
        .max_rows               = 25,
        .max_cols               = 80,
    };
}

fk::core::Result<void, fk::core::Error>
SerialTerminal::set_size(uint16_t rows, uint16_t cols) {
    if (rows > 0) m_rows = rows;
    if (cols > 0) m_cols = cols;
    return {};
}

void SerialTerminal::get_size(uint16_t& rows, uint16_t& cols) const {
    rows = m_rows;
    cols = m_cols;
}

} // namespace terminal
} // namespace fkernel
