#include <Kernel/Driver/Pty/pty_master.h>

namespace fkernel {

PtyMaster::PtyMaster(fk::RefPtr<PtyBuffer> to_slave,
                     fk::RefPtr<PtyBuffer> from_slave)
    : m_to_slave(to_slave), m_from_slave(from_slave) {
  set_name("ptm");
}

fk::core::Result<size_t, fk::core::Error>
PtyMaster::read(uint64_t, size_t size, uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;
  if (m_from_slave->is_empty()) return (size_t)0;
  return m_from_slave->read(buf, size);
}

fk::core::Result<size_t, fk::core::Error>
PtyMaster::write(uint64_t, size_t size, const uint8_t* buf) {
  if (!buf || size == 0) return fk::core::Error::InvalidParameter;
  size_t written = m_to_slave->write(buf, size);
  return written;
}

} // namespace fkernel
