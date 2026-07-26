#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr size_t NCCS = 32;

struct Termios {
  uint32_t c_iflag;
  uint32_t c_oflag;
  uint32_t c_cflag;
  uint32_t c_lflag;
  uint8_t  c_cc[NCCS];

  static constexpr uint32_t ICANON  = 0x00000002;
  static constexpr uint32_t ECHO    = 0x00000008;
  static constexpr uint32_t ECHOE   = 0x00000010;
  static constexpr uint32_t ISIG    = 0x00000001;
  static constexpr uint32_t ECHONL  = 0x00000040;

  static constexpr uint8_t VINTR  = 0;
  static constexpr uint8_t VQUIT  = 1;
  static constexpr uint8_t VERASE = 2;
  static constexpr uint8_t VKILL  = 3;
  static constexpr uint8_t VEOF   = 4;
  static constexpr uint8_t VEOL   = 5;
  static constexpr uint8_t VSUSP  = 10;
  static constexpr uint8_t VSTART = 11;
  static constexpr uint8_t VSTOP  = 12;

  void set_defaults() {
    c_iflag = 0x2502;
    c_oflag = 0x0005;
    c_cflag = 0x04cb;
    c_lflag = ICANON | ECHO | ECHOE | ISIG | ECHONL;
    for (size_t i = 0; i < NCCS; ++i) c_cc[i] = 0;
    c_cc[VINTR]  = 0x03; // ^C
    c_cc[VQUIT]  = 0x1C; // backslash
    c_cc[VERASE] = 0x7F; // DEL
    c_cc[VKILL]  = 0x15; // Ctrl-U
    c_cc[VEOF]   = 0x04; // ^D
    c_cc[VSUSP]  = 0x1A; // ^Z
  }

  bool has_lflag(uint32_t flag) const { return (c_lflag & flag) != 0; }
};

class PtyLineDiscipline {
public:
  PtyLineDiscipline();

  void set_termios(const Termios& t);
  const Termios& termios() const { return m_termios; }

  // Process a byte from master→slave path. Returns >0 if byte should be
  // delivered to slave, 0 if consumed (e.g., echo), or <0 for signal.
  // out_echo is set to true when the byte should be echoed back.
  int process_input(uint8_t byte, bool* out_echo);

  // Process a byte from slave→master path. Returns the byte to deliver.
  int process_output(uint8_t byte);

  // Signal to send based on last processed char (VINTR, VQUIT, VSUSP).
  int pending_signal() const { return m_pending_signal; }
  void clear_pending_signal() { m_pending_signal = 0; }

private:
  Termios m_termios;
  int m_pending_signal{0};
};

}
