#include <Kernel/Driver/Pty/pty_line_discipline.h>

namespace fkernel {

PtyLineDiscipline::PtyLineDiscipline() {
  m_termios.set_defaults();
}

void PtyLineDiscipline::set_termios(const Termios& t) {
  m_termios = t;
}

int PtyLineDiscipline::process_input(uint8_t byte, bool* out_echo) {
  *out_echo = false;
  m_pending_signal = 0;

  if (m_termios.has_lflag(Termios::ISIG)) {
    if (byte == m_termios.c_cc[Termios::VINTR]) {
      m_pending_signal = 2;
      return 0;
    }
    if (byte == m_termios.c_cc[Termios::VQUIT]) {
      m_pending_signal = 3;
      return 0;
    }
    if (byte == m_termios.c_cc[Termios::VSUSP]) {
      m_pending_signal = 20;
      return 0;
    }
  }

  if (m_termios.has_lflag(Termios::ECHO)) {
    *out_echo = true;
    if (byte == m_termios.c_cc[Termios::VERASE] && m_termios.has_lflag(Termios::ECHOE)) {
      *out_echo = true;
    }
  }

  return 1;
}

int PtyLineDiscipline::process_output(uint8_t byte) {
  return static_cast<int>(byte);
}

}
