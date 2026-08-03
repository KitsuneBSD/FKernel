#include <Kernel/Net/Sockets/unix_socket_buffer.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

UnixSocketBuffer::UnixSocketBuffer() {
  m_data = reinterpret_cast<uint8_t*>(MemoryManager::the().allocate(CAPACITY));
}

UnixSocketBuffer::~UnixSocketBuffer() {
  if (m_data)
    MemoryManager::the().free(m_data);
}

size_t UnixSocketBuffer::available() const {
  return m_write_ptr - m_read_ptr;
}

size_t UnixSocketBuffer::free_space() const {
  return CAPACITY - available();
}

size_t UnixSocketBuffer::read(uint8_t* buf, size_t max) {
  size_t to_read = max < available() ? max : available();
  if (to_read == 0)
    return 0;
  size_t rpos = m_read_ptr % CAPACITY;
  size_t first = CAPACITY - rpos;
  if (first >= to_read) {
    fk::memory::copy(buf, m_data + rpos, to_read);
    m_read_ptr += to_read;
    return to_read;
  }
  fk::memory::copy(buf, m_data + rpos, first);
  fk::memory::copy(buf + first, m_data, to_read - first);
  m_read_ptr += to_read;
  return to_read;
}

size_t UnixSocketBuffer::write(const uint8_t* src, size_t count) {
  size_t to_write = count < free_space() ? count : free_space();
  if (to_write == 0)
    return 0;
  size_t wpos = m_write_ptr % CAPACITY;
  size_t first = CAPACITY - wpos;
  if (first >= to_write) {
    fk::memory::copy(m_data + wpos, src, to_write);
    m_write_ptr += to_write;
    return to_write;
  }
  fk::memory::copy(m_data + wpos, src, first);
  fk::memory::copy(m_data, src + first, to_write - first);
  m_write_ptr += to_write;
  return to_write;
}

} // namespace fkernel
