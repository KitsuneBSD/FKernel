#include <Kernel/Net/unix_socket_buffer.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibC/string.h>

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
  memcpy(buf, m_data + (m_read_ptr % CAPACITY), to_read);
  m_read_ptr += to_read;
  return to_read;
}

size_t UnixSocketBuffer::write(const uint8_t* src, size_t count) {
  size_t to_write = count < free_space() ? count : free_space();
  if (to_write == 0)
    return 0;
  memcpy(m_data + (m_write_ptr % CAPACITY), src, to_write);
  m_write_ptr += to_write;
  return to_write;
}

} // namespace fkernel
