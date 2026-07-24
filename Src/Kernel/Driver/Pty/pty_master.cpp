#include <Kernel/Driver/Pty/pty_master.h>

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
  size_t written = m_to_slave->write(buf, size);
  return written;
}

fk::core::Result<int, fk::core::Error>
PtyMaster::ioctl(uint64_t request, uint64_t arg) {
  static constexpr uint64_t TIOCGPTN = 0x80045430;
  if (request != TIOCGPTN) return fk::core::Error::NotImplemented;
  // Parse index from name "ptm{n}"
  const char* p = name().c_str() + 3; // skip "ptm"
  unsigned int n = 0;
  while (*p >= '0' && *p <= '9') n = n * 10 + (unsigned int)(*p++ - '0');
  *reinterpret_cast<unsigned int*>(arg) = n;
  return 0;
}

} // namespace fkernel
