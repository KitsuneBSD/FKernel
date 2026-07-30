#include <Kernel/Driver/Pty/pty_slave.h>
#include <LibFK/Text/string.h>

namespace fkernel {

PtySlave::PtySlave(fk::RefPtr<PtyBuffer> from_master,
                   fk::RefPtr<PtyBuffer> to_master,
                   uint32_t index, PtyLineDiscipline* ldisc)
    : m_from_master(from_master), m_to_master(to_master), m_ldisc(ldisc) {
  fk::text::String name("pts");
  char idx_buf[16];
  size_t i = 0;
  uint32_t v = index;
  if (v == 0) { idx_buf[i++] = '0'; }
  while (v > 0) { idx_buf[i++] = '0' + (char)(v % 10); v /= 10; }
  for (size_t l = 0, r = i - 1; l < r; ++l, --r) {
    char tmp = idx_buf[l]; idx_buf[l] = idx_buf[r]; idx_buf[r] = tmp;
  }
  idx_buf[i] = '\0';
  name.append(idx_buf);
  set_name(fk::types::move(name));
}

fk::core::Result<size_t, fk::core::Error>
PtySlave::read(uint64_t, size_t size, uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;
  while (m_from_master->is_empty())
    TRY(m_from_master->data_ready().wait_interruptible());
  return m_from_master->read(buf, size);
}

fk::core::Result<size_t, fk::core::Error>
PtySlave::write(uint64_t, size_t size, const uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;
  bool opost = m_ldisc && m_ldisc->termios().has_oflag(Termios::OPOST);
  bool onlcr = opost && m_ldisc->termios().has_oflag(Termios::ONLCR);
  if (!onlcr)
    return m_to_master->write(buf, size);
  // Translate \n → \r\n for each byte
  for (size_t i = 0; i < size; ++i) {
    if (buf[i] == '\n') {
      static const uint8_t crlf[2] = {'\r', '\n'};
      m_to_master->write(crlf, 2);
    } else {
      m_to_master->write(buf + i, 1);
    }
  }
  return size;
}

} // namespace fkernel
