#include <Kernel/Driver/Pty/pty_line_discipline.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

PtyLineDiscipline::PtyLineDiscipline() {
  m_termios.set_defaults();
}

void PtyLineDiscipline::set_termios(const Termios& t) {
  m_termios = t;
}

void PtyLineDiscipline::echo_push(uint8_t b) {
  if (m_echobuf_len < sizeof(m_echobuf))
    m_echobuf[m_echobuf_len++] = b;
}

void PtyLineDiscipline::echo_push_str(const uint8_t* s, size_t n) {
  for (size_t i = 0; i < n; ++i) echo_push(s[i]);
}

void PtyLineDiscipline::process_input(uint8_t byte) {
  m_pending_signal = 0;
  m_raw_ready      = false;

  // Signal characters (ISIG) — handled in all modes
  if (m_termios.has_lflag(Termios::ISIG)) {
    if (byte == m_termios.c_cc[Termios::VINTR]) { m_pending_signal = 2; return; }
    if (byte == m_termios.c_cc[Termios::VQUIT]) { m_pending_signal = 3; return; }
    if (byte == m_termios.c_cc[Termios::VSUSP]) { m_pending_signal = 20; return; }
  }

  bool do_echo = m_termios.has_lflag(Termios::ECHO);

  if (!m_termios.has_lflag(Termios::ICANON)) {
    // Raw mode: deliver byte immediately
    m_raw_ready = true;
    m_raw_byte  = byte;
    if (do_echo) echo_push(byte);
    return;
  }

  // ICANON mode
  if (byte == m_termios.c_cc[Termios::VERASE]) {
    // Backspace: erase last character
    if (m_linebuf_len > 0) {
      --m_linebuf_len;
      if (do_echo && m_termios.has_lflag(Termios::ECHOE)) {
        static const uint8_t bs_sp_bs[3] = {'\b', ' ', '\b'};
        echo_push_str(bs_sp_bs, 3);
      }
    }
    return;
  }

  if (byte == m_termios.c_cc[Termios::VKILL]) {
    // Kill line: erase the whole line
    if (do_echo) {
      // Erase all buffered characters
      for (size_t i = 0; i < m_linebuf_len; ++i) {
        static const uint8_t bs_sp_bs[3] = {'\b', ' ', '\b'};
        echo_push_str(bs_sp_bs, 3);
      }
    }
    m_linebuf_len = 0;
    return;
  }

  if (byte == '\n' || byte == m_termios.c_cc[Termios::VEOL]) {
    // End of line: add newline and mark line as ready
    if (m_linebuf_len < MAX_CANON)
      m_linebuf[m_linebuf_len++] = byte;
    if (do_echo && m_termios.has_lflag(Termios::ECHONL))
      echo_push('\n');
    m_line_ready = true;
    return;
  }

  if (byte == m_termios.c_cc[Termios::VEOF]) {
    // EOF (Ctrl-D): deliver current buffer contents without adding newline
    m_line_ready = true;
    return;
  }

  // Regular character: append to buffer
  if (m_linebuf_len < MAX_CANON) {
    m_linebuf[m_linebuf_len++] = byte;
    if (do_echo) echo_push(byte);
  }
}

size_t PtyLineDiscipline::drain_canonical_line(uint8_t* buf, size_t max) {
  size_t n = (m_linebuf_len < max) ? m_linebuf_len : max;
  fk::memory::copy(buf, m_linebuf, n);
  m_linebuf_len = 0;
  m_line_ready  = false;
  return n;
}

size_t PtyLineDiscipline::drain_echo(uint8_t* buf, size_t max) {
  size_t n = (m_echobuf_len < max) ? m_echobuf_len : max;
  fk::memory::copy(buf, m_echobuf, n);
  m_echobuf_len = 0;
  return n;
}

int PtyLineDiscipline::process_output(uint8_t byte) {
  return static_cast<int>(byte);
}

}
