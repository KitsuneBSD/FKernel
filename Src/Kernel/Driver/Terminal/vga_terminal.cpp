#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Keyboard/keymap_manager.h>
#include <Kernel/Ipc/Signals/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {
namespace terminal {

static VGATerminal* s_vga_tty = nullptr;

VGATerminal::VGATerminal(int index) : m_index(index), m_ansi_parser(*this), m_renderer(25, 80) {
  if (index == 0)
    s_vga_tty = this;
  m_state.rows = static_cast<uint16_t>(Display::the().get_height());
  m_state.cols = static_cast<uint16_t>(Display::the().get_width());

  char name[16];
  snprintf(name, sizeof(name), "tty%d", index);
  set_name(fk::text::String(name));
}

VGATerminal& VGATerminal::the() {
  if (s_vga_tty)
    return *s_vga_tty;
  static VGATerminal fallback(0);
  return fallback;
}

void VGATerminal::on_char(char c) {
  fk::synchronization::ScopedLock lock(m_lock);
  bool is_active = (TerminalManager::the().active_terminal() == this);

  // Control character signal delivery (ISIG)
  if (m_state.isig_enabled && m_state.foreground_pgid > 0) {
    if (c == '\x03') { // Ctrl+C → SIGINT
      fk::algorithms::kdebug("TTY", "SIGINT to pgrp %d (active=%d)", m_state.foreground_pgid, is_active);
      SchedulerManager::the().send_signal_to_pgrp(m_state.foreground_pgid, SIGINT);
      return;
    }
    if (c == '\x1C') { // Ctrl+\ → SIGQUIT
      fk::algorithms::kdebug("TTY", "SIGQUIT to pgrp %d", m_state.foreground_pgid);
      SchedulerManager::the().send_signal_to_pgrp(m_state.foreground_pgid, SIGQUIT);
      return;
    }
    if (c == '\x1A') { // Ctrl+Z → SIGTSTP
      fk::algorithms::kdebug("TTY", "SIGTSTP to pgrp %d", m_state.foreground_pgid);
      SchedulerManager::the().send_signal_to_pgrp(m_state.foreground_pgid, SIGTSTP);
      return;
    }
  }

  // Log ISIG miss: char 0x03 received but not delivered
  if (c == '\x03' && m_state.isig_enabled && m_state.foreground_pgid <= 0) {
    fk::algorithms::kdebug("TTY", "SIGINT dropped: no foreground_pgid (isig=%d pgid=%d)", m_state.isig_enabled, m_state.foreground_pgid);
  }

  // Ctrl+D (EOF): in canonical mode with empty line buffer, signal EOF to reader
  if (c == '\x04' && !m_state.raw_mode) {
    if (m_input_queue.is_empty()) {
      m_state.eof_pending = true;
      m_read_endpoint.signal(fk::NotificationBits(1));
    }
    return;
  }

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
    m_read_endpoint.signal(fk::NotificationBits(1));
    return;
  }

  if (!m_state.raw_mode)
    m_state.line_chars++;
  if (m_state.echo_enabled && !m_state.raw_mode && is_active) {
    m_renderer.put_char(c);
    Display::the().flush();
  }
  m_input_queue.enqueue(c);
  m_read_endpoint.signal(fk::NotificationBits(1));
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::read(uint64_t, size_t size,
                                                            uint8_t* buffer) {
  if (size == 0 || !buffer)
    return fk::core::Error::InvalidParameter;

  // SIGTTIN: block background processes from reading the terminal
  if (m_state.foreground_pgid > 0) {
    auto* task = SchedulerManager::the().current();
    if (task && !task->control.lifecycle.is_a_kernel_task &&
        (int)task->control.identity.pgid.value() != m_state.foreground_pgid) {
      fkernel::ipc::SignalDelivery::send_signal(task, SIGTTIN);
      return fk::core::Error::Interrupted;
    }
  }

  if (m_state.raw_mode) {
    while (true) {
      {
        fk::synchronization::ScopedLockIRQ lock(m_lock);
        if (!m_input_queue.is_empty()) {
          buffer[0] = static_cast<uint8_t>(m_input_queue.dequeue());
          return 1;
        }
      }
      TRY(m_read_endpoint.wait_interruptible());
    }
  }

  // Canonical mode: wait for full line
  while (true) {
    {
      fk::synchronization::ScopedLockIRQ lock(m_lock);
      if (m_state.eof_pending && m_input_queue.is_empty()) {
        m_state.eof_pending = false;
        return 0;
      }
      for (size_t i = 0; i < m_input_queue.size(); ++i) {
        if (m_input_queue[i] == '\n') {
          size_t to_copy = (i + 1 < size) ? (i + 1) : size;
          for (size_t j = 0; j < to_copy; ++j)
            buffer[j] = m_input_queue.dequeue();
          return to_copy;
        }
      }
    }
    TRY(m_read_endpoint.wait_interruptible());
  }
}

fk::core::Result<size_t, fk::core::Error> VGATerminal::write(uint64_t, size_t size,
                                                             const uint8_t* buffer) {
  if (!buffer)
    return fk::core::Error::InvalidParameter;

  // SIGTTOU: block background processes from writing if TOSTOP is set.
  // For simplicity we always deliver SIGTTOU to background writers.
  if (m_state.foreground_pgid > 0) {
    auto* task = SchedulerManager::the().current();
    if (task && !task->control.lifecycle.is_a_kernel_task &&
        (int)task->control.identity.pgid.value() != m_state.foreground_pgid) {
      fkernel::ipc::SignalDelivery::send_signal(task, SIGTTOU);
      return fk::core::Error::Interrupted;
    }
  }

  fk::synchronization::ScopedLockIRQ lock(m_lock);
  if (TerminalManager::the().active_terminal() != this) {
    fk::algorithms::kwarn("VGATERM", "write to inactive terminal tty%d (%zu bytes discarded)",
                          m_index, size);
    return size;
  }
  m_ansi_parser.process_data(reinterpret_cast<const char*>(buffer), size);
  Display::the().flush();
  return size;
}

// Delegation to Renderer
void VGATerminal::put_char(char c) {
  m_renderer.put_char(c);
}
void VGATerminal::clear() {
  m_renderer.clear();
}
void VGATerminal::move_cursor(uint16_t r, uint16_t c) {
  m_renderer.move_cursor(r, c);
}
void VGATerminal::set_colors(uint8_t f, uint8_t b) {
  m_renderer.set_colors(f, b);
}
void VGATerminal::set_colors_rgb(uint32_t f, uint32_t b) {
  m_renderer.set_colors_rgb(f, b);
}
void VGATerminal::clear_screen(uint8_t m) {
  m_renderer.clear_screen(m, m_state.rows);
}
void VGATerminal::clear_line(uint8_t m) {
  m_renderer.clear_line(m, m_state.rows, m_state.cols);
}
void VGATerminal::save_cursor() {
  m_renderer.save_cursor();
}
void VGATerminal::restore_cursor() {
  m_renderer.restore_cursor();
}
void VGATerminal::scroll_up(uint16_t n) {
  m_renderer.scroll_up(n, m_state.rows);
}
void VGATerminal::scroll_down(uint16_t n) {
  m_renderer.scroll_down(n, m_state.rows);
}
void VGATerminal::insert_chars(uint16_t n) {
  m_renderer.insert_chars(n, m_state.rows, m_state.cols);
}
void VGATerminal::delete_chars(uint16_t n) {
  m_renderer.delete_chars(n, m_state.rows, m_state.cols);
}
void VGATerminal::erase_chars(uint16_t n) {
  m_renderer.erase_chars(n, m_state.rows, m_state.cols);
}
void VGATerminal::insert_lines(uint16_t n) {
  m_renderer.insert_lines(n, m_state.rows);
}
void VGATerminal::delete_lines(uint16_t n) {
  m_renderer.delete_lines(n, m_state.rows);
}

// State accessors
uint32_t VGATerminal::get_cursor_x() {
  return m_renderer.cursor_x();
}
uint32_t VGATerminal::get_cursor_y() {
  return m_renderer.cursor_y();
}
uint16_t VGATerminal::get_width() {
  return m_state.cols;
}
uint16_t VGATerminal::get_height() {
  return m_state.rows;
}

void VGATerminal::set_active(VGATerminal* terminal) {
  s_vga_tty = terminal;
}
const char* VGATerminal::type_name() const {
  return "VGA";
}
fk::core::Result<void, fk::core::Error> VGATerminal::attach_input(InputDevice*) {
  return {};
}
fk::core::Result<void, fk::core::Error> VGATerminal::attach_output(OutputDevice*) {
  return {};
}

TerminalCapabilities VGATerminal::capabilities() const {
  return {.supports_color = true,
          .supports_raw_mode = true,
          .supports_canonical_mode = true,
          .max_rows = m_state.rows,
          .max_cols = m_state.cols};
}

fk::core::Result<void, fk::core::Error> VGATerminal::set_size(uint16_t r, uint16_t c) {
  m_state.rows = r;
  m_state.cols = c;
  return {};
}

void VGATerminal::get_size(uint16_t& r, uint16_t& c) const {
  r = m_state.rows;
  c = m_state.cols;
}

void VGATerminal::respond(const char* data) {
  if (!data)
    return;
  while (*data)
    m_input_queue.enqueue(*data++);
}

void VGATerminal::move_cursor_up(uint16_t n) {
  move_cursor(m_renderer.cursor_y() + 1 - n, m_renderer.cursor_x() + 1);
}
void VGATerminal::move_cursor_down(uint16_t n) {
  move_cursor(m_renderer.cursor_y() + 1 + n, m_renderer.cursor_x() + 1);
}
void VGATerminal::move_cursor_forward(uint16_t n) {
  move_cursor(m_renderer.cursor_y() + 1, m_renderer.cursor_x() + 1 + n);
}
void VGATerminal::move_cursor_back(uint16_t n) {
  move_cursor(m_renderer.cursor_y() + 1, m_renderer.cursor_x() + 1 - n);
}
void VGATerminal::save_screen() {
  Display::the().save_screen();
}
void VGATerminal::restore_screen() {
  Display::the().restore_screen();
}
void VGATerminal::show_cursor(bool v) {
  Display::the().show_cursor(v);
}
void VGATerminal::set_scroll_region(uint16_t, uint16_t) {}
void VGATerminal::set_line_drawing_mode(bool e) {
  m_state.line_drawing_mode = e;
}

fk::core::Result<int, fk::core::Error> VGATerminal::ioctl(uint64_t request, uint64_t arg) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  // Linux struct termios (x86_64 ABI: 4x uint32 + cc_t[19])
  struct linux_termios {
    unsigned int c_iflag, c_oflag, c_cflag, c_lflag;
    unsigned char c_line;
    unsigned char c_cc[19];
  };
  static constexpr unsigned int ICANON = 0x2;
  static constexpr unsigned int ECHO   = 0x8;
  static constexpr unsigned int ISIG   = 0x1;

  if (request == 0x5401 /* TCGETS */) {
    // Build termios in kernel memory, copy to user
    struct linux_termios kt;
    fk::memory::set(&kt, 0, sizeof(kt));
    kt.c_cflag = 0xBF;  // B9600 | CS8
    if (m_state.echo_enabled) kt.c_lflag |= ECHO;
    if (!m_state.raw_mode)    kt.c_lflag |= ICANON;
    if (m_state.isig_enabled) kt.c_lflag |= ISIG;
    kt.c_cc[4] = 1;  // VMIN
    kt.c_cc[5] = 0;  // VTIME
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &kt, sizeof(kt)).is_error())
      return fk::core::Error::InvalidParameter;
    return 0;
  }
  if (request == 0x5402 /* TCSETS */ || request == 0x5403 /* TCSETSW */ ||
      request == 0x5404 /* TCSETSF */) {
    struct linux_termios kt;
    if (fkernel::memory::copy_from_user(&kt, reinterpret_cast<const void*>(arg), sizeof(kt)).is_error())
      return fk::core::Error::InvalidParameter;
    m_state.echo_enabled = (kt.c_lflag & ECHO)   != 0;
    m_state.raw_mode     = (kt.c_lflag & ICANON) == 0;
    m_state.isig_enabled = (kt.c_lflag & ISIG)   != 0;
    return 0;
  }
  if (request == 0x5413 /* TIOCGWINSZ */) {
    struct ws { uint16_t r, c, x, y; } w;
    w.r = m_state.rows;
    w.c = m_state.cols;
    w.x = 0;
    w.y = 0;
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &w, sizeof(w)).is_error())
      return fk::core::Error::InvalidParameter;
    return 0;
  }
  if (request == 0x540F /* TIOCGPGRP */) {
    int pgid = m_state.foreground_pgid;
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &pgid, sizeof(pgid)).is_error())
      return fk::core::Error::InvalidParameter;
    return 0;
  }
  if (request == 0x5410 /* TIOCSPGRP */) {
    int pgid;
    if (fkernel::memory::copy_from_user(&pgid, reinterpret_cast<const void*>(arg), sizeof(pgid)).is_error())
      return fk::core::Error::InvalidParameter;
    m_state.foreground_pgid = pgid;
    return 0;
  }
  if (request == 0x540E /* TIOCSCTTY */) {
    Task* task = SchedulerManager::the().current();
    if (task) m_state.foreground_pgid = (int)task->control.identity.pgid.value();
    return 0;
  }
  // Custom keyboard ioctls (0x4Bxx)
  if (request == 0x4B01 /* KBDIO_SETLAYOUT */) {
    auto layout = static_cast<fkernel::drivers::KeyboardLayout>(arg);
    fkernel::drivers::KeymapManager::the().set_layout(layout);
    return 0;
  }
  if (request == 0x4B02 /* KBDIO_GETLAYOUT */) {
    return static_cast<int>(fkernel::drivers::KeymapManager::the().layout());
  }
  if (request == 0x4B03 /* KBDIO_SETCOMPOSE */) {
    fkernel::drivers::KeymapManager::the().set_compose_mode(arg != 0);
    return 0;
  }
  return fk::core::Error::InappropriateIoctlForDevice;
}

} // namespace terminal
} // namespace fkernel