#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/Memory.h>

using namespace fk::core;

namespace fkernel {

fk::core::Result<size_t, fk::core::Error> TtyNode::read(uint64_t, size_t size,
                                                        uint8_t *buffer) {
  if (!buffer)
    return fk::core::Error::InvalidParameter;

  size_t copied = 0;
  while (copied < size) {
    // Block until we have at least one key
    while (!PS2Keyboard::the().has_key()) {
      SchedulerManager::the().schedule();
    }

    char c = PS2Keyboard::the().pop_key();
    if (!c)
      continue;

    // Handle Backspace
    if (c == '\b' || c == 127) {
      if (copied > 0) {
        copied--;
        display::the().put_char('\b');
        serial::write_char('\b');
      }
      continue;
    }

    // Echo character
    display::the().put_char(c);
    serial::write_char(c);

    buffer[copied++] = static_cast<uint8_t>(c);
    // Stop at newline to emulate line-oriented TTY
    if (c == '\n')
      break;
  }

  return copied;
}

fk::core::Result<size_t, fk::core::Error>
TtyNode::write(uint64_t, size_t size, const uint8_t *buffer) {
  if (!buffer)
    return fk::core::Error::InvalidParameter;

  display::the().write_ansi_n(reinterpret_cast<const char *>(buffer), size);

  // Also log to serial
  for (size_t i = 0; i < size; ++i) {
    serial::write_char(static_cast<char>(buffer[i]));
  }

  return size;
}

struct winsize {
  uint16_t ws_row;
  uint16_t ws_col;
  uint16_t ws_xpixel;
  uint16_t ws_ypixel;
};

struct termios {
  uint32_t c_iflag;
  uint32_t c_oflag;
  uint32_t c_cflag;
  uint32_t c_lflag;
  uint8_t c_line;
  uint8_t c_cc[32];
};

#define TIOCGWINSZ 0x5413
#define TCGETS 0x5401

fk::core::Result<int, fk::core::Error> TtyNode::ioctl(uint64_t request,
                                                      uint64_t arg) {
  if (request == TIOCGWINSZ) {
    winsize *ws = reinterpret_cast<winsize *>(arg);
    ws->ws_row = 25;
    ws->ws_col = 80;
    ws->ws_xpixel = 0;
    ws->ws_ypixel = 0;
    return 0;
  }

  if (request == TCGETS) {
    termios *t = reinterpret_cast<termios *>(arg);
    fk::memory::set(t, 0, sizeof(termios));
    // Sane defaults: ICANON | ECHO | ISIG
    t->c_lflag = 0x0000000a | 0x00000001;
    return 0;
  }

  return fk::core::Error::NotImplemented;
}

} // namespace fkernel
