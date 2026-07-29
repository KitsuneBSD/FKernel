#include <Kernel/Driver/Pty/pty_master.h>
#include <Kernel/Ipc/signal_delivery.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>

struct winsize {
  uint16_t ws_row;
  uint16_t ws_col;
  uint16_t ws_xpixel;
  uint16_t ws_ypixel;
};

namespace fkernel {

static void uint32_to_str(uint32_t v, char* buf) {
  if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
  char tmp[12]; int len = 0;
  while (v > 0) { tmp[len++] = '0' + (char)(v % 10); v /= 10; }
  for (int i = 0; i < len; ++i) buf[i] = tmp[len - 1 - i];
  buf[len] = '\0';
}

PtyMaster::PtyMaster(fk::RefPtr<PtyBuffer> to_slave,
                     fk::RefPtr<PtyBuffer> from_slave,
                     uint32_t index)
    : m_to_slave(to_slave), m_from_slave(from_slave) {
  char name_buf[16] = "ptm";
  uint32_to_str(index, name_buf + 3);
  set_name(fk::text::String(name_buf));
}

fk::core::Result<size_t, fk::core::Error>
PtyMaster::read(uint64_t, size_t size, uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;
  while (m_from_slave->is_empty())
    m_from_slave->data_ready().wait();
  return m_from_slave->read(buf, size);
}

fk::core::Result<size_t, fk::core::Error>
PtyMaster::write(uint64_t, size_t size, const uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;

  bool icanon = m_ldisc.termios().has_lflag(Termios::ICANON);

  for (size_t i = 0; i < size; ++i) {
    m_ldisc.process_input(buf[i]);

    int sig = m_ldisc.pending_signal();
    if (sig) {
      m_ldisc.clear_pending_signal();
      auto* task = SchedulerManager::the().current();
      if (task) ipc::SignalDelivery::send_signal(task, sig);
    }

    // Drain echo bytes back to terminal (master reads these)
    if (m_ldisc.echo_available()) {
      uint8_t echobuf[MAX_CANON * 3];
      size_t n = m_ldisc.drain_echo(echobuf, sizeof(echobuf));
      m_from_slave->write(echobuf, n);
    }

    // Deliver data to slave
    if (icanon) {
      if (m_ldisc.has_canonical_line()) {
        uint8_t linebuf[MAX_CANON];
        size_t n = m_ldisc.drain_canonical_line(linebuf, sizeof(linebuf));
        m_to_slave->write(linebuf, n);
      }
    } else {
      if (m_ldisc.has_raw_byte()) {
        uint8_t c = m_ldisc.drain_raw_byte();
        m_to_slave->write(&c, 1);
      }
    }
  }
  return size;
}

fk::core::Result<int, fk::core::Error>
PtyMaster::ioctl(uint64_t request, uint64_t arg) {
  static constexpr uint64_t TIOCGPTN   = 0x80045430;
  static constexpr uint64_t TCSETS     = 0x5402;
  static constexpr uint64_t TCGETS     = 0x5401;
  static constexpr uint64_t TIOCGWINSZ = 0x5413;
  static constexpr uint64_t TIOCSWINSZ = 0x5414;
  static constexpr uint64_t TIOCSCTTY  = 0x540E;
  static constexpr uint64_t TIOCGPGRP  = 0x540F;
  static constexpr uint64_t TIOCSPGRP  = 0x5410;

  if (request == TIOCGPTN) {
    const char* p = name().c_str() + 3;
    unsigned int n = 0;
    while (*p >= '0' && *p <= '9') n = n * 10 + (unsigned int)(*p++ - '0');
    fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &n, sizeof(n));
    return 0;
  }

  if (request == TIOCGWINSZ) {
    struct winsize ws;
    ws.ws_row = m_rows;
    ws.ws_col = m_cols;
    ws.ws_xpixel = m_cols * 8;
    ws.ws_ypixel = m_rows * 16;
    fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &ws, sizeof(ws));
    return 0;
  }

  if (request == TIOCSWINSZ) {
    struct winsize ws;
    if (fkernel::memory::copy_from_user(&ws, reinterpret_cast<const void*>(arg), sizeof(ws)).is_error())
      return fk::core::Error::InvalidParameter;
    if (ws.ws_row > 0) m_rows = ws.ws_row;
    if (ws.ws_col > 0) m_cols = ws.ws_col;
    return 0;
  }

  if (request == TIOCSCTTY) {
    auto* task = SchedulerManager::the().current();
    if (!task) return fk::core::Error::PermissionDenied;
    m_foreground_pgid = static_cast<int>(task->control.identity.pgid.value());
    return 0;
  }

  if (request == TIOCGPGRP) {
    int pgid = m_foreground_pgid;
    if (fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &pgid, sizeof(pgid)).is_error())
      return fk::core::Error::InvalidParameter;
    return 0;
  }

  if (request == TIOCSPGRP) {
    int pgid = 0;
    if (fkernel::memory::copy_from_user(&pgid, reinterpret_cast<const void*>(arg), sizeof(pgid)).is_error())
      return fk::core::Error::InvalidParameter;
    m_foreground_pgid = pgid;
    return 0;
  }

  if (request == TCGETS) {
    fkernel::memory::copy_to_user(reinterpret_cast<void*>(arg), &m_ldisc.termios(), sizeof(Termios));
    return 0;
  }

  if (request == TCSETS) {
    Termios t;
    if (fkernel::memory::copy_from_user(&t, reinterpret_cast<const void*>(arg), sizeof(Termios)).is_error())
      return fk::core::Error::InvalidParameter;
    m_ldisc.set_termios(t);
    return 0;
  }

  return fk::core::Error::NotImplemented;
}

}
