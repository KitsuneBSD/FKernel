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
  bool has_oflag(uint32_t flag) const { return (c_oflag & flag) != 0; }

  static constexpr uint32_t OPOST = 0x00000001;
  static constexpr uint32_t ONLCR = 0x00000004; // map NL to CR-NL on output
};

static constexpr size_t MAX_CANON = 1024;

class PtyLineDiscipline {
public:
  PtyLineDiscipline();

  void set_termios(const Termios& t);
  const Termios& termios() const { return m_termios; }

  // Process a byte from master→slave path.
  // In ICANON mode: buffers until a complete line; in raw mode: delivers immediately.
  void process_input(uint8_t byte);

  // Process a byte from slave→master path. Returns the (possibly translated) byte.
  int process_output(uint8_t byte);

  // ICANON mode: returns true when a complete line is ready for reading.
  bool has_canonical_line() const { return m_line_ready; }

  // Drain up to `max` bytes of the canonical line into `buf`. Clears line buffer.
  size_t drain_canonical_line(uint8_t* buf, size_t max);

  // Raw mode: bytes available immediately.
  bool has_raw_byte() const { return m_raw_ready; }
  uint8_t drain_raw_byte() { m_raw_ready = false; return m_raw_byte; }

  // Bytes pending for echo back to the terminal master.
  size_t echo_available() const { return m_echobuf_len; }
  size_t drain_echo(uint8_t* buf, size_t max);

  // Signal to send based on last processed char (VINTR, VQUIT, VSUSP).
  int pending_signal() const { return m_pending_signal; }
  void clear_pending_signal() { m_pending_signal = 0; }

private:
  Termios m_termios;
  int m_pending_signal{0};

  // ICANON line buffer
  uint8_t m_linebuf[MAX_CANON]{};
  size_t  m_linebuf_len{0};
  bool    m_line_ready{false};

  // Echo buffer (up to 3 bytes per input char for BS-SP-BS sequences)
  uint8_t m_echobuf[MAX_CANON * 3]{};
  size_t  m_echobuf_len{0};

  // Raw mode: last processed byte
  bool    m_raw_ready{false};
  uint8_t m_raw_byte{0};

  void echo_push(uint8_t b);
  void echo_push_str(const uint8_t* s, size_t n);
};

}
