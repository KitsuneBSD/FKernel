#pragma once

#include <Kernel/Driver/Pty/termios.h>
#include <LibFK/Types/types.h>

namespace fkernel {

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

  // VLNEXT: next input char is literal (bypasses special-char processing)
  bool    m_lnext{false};

  void echo_push(uint8_t b);
  void echo_push_str(const uint8_t* s, size_t n);
};

}

