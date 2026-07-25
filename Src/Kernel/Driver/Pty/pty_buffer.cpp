#include <Kernel/Driver/Pty/pty_buffer.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

size_t PtyBuffer::read(uint8_t* dst, size_t len) {
  size_t n = m_data.size() < len ? m_data.size() : len;
  if (n == 0) return 0;
  fk::memory::copy(dst, m_data.begin(), n);
  for (size_t i = n; i < m_data.size(); ++i)
    m_data[i - n] = m_data[i];
  for (size_t i = 0; i < n; ++i)
    m_data.pop_back();
  return n;
}

size_t PtyBuffer::write(const uint8_t* src, size_t len) {
  size_t space = CAPACITY - m_data.size();
  size_t n = len < space ? len : space;
  for (size_t i = 0; i < n; ++i)
    m_data.push_back(src[i]);
  if (n > 0) m_notification.signal(1);
  return n;
}

bool PtyBuffer::is_empty() const { return m_data.is_empty(); }
size_t PtyBuffer::available() const { return m_data.size(); }

} // namespace fkernel
