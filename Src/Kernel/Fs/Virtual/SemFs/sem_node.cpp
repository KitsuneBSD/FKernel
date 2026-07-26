#include <Kernel/Fs/Virtual/SemFs/sem_node.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<SemNode>, fk::core::Error>
SemNode::create(uint32_t value, uint32_t max) {
  if (max == 0) max = ~0u;
  if (value > max) return fk::core::Error::InvalidParameter;
  return fk::RefPtr<SemNode>(new SemNode(value, max));
}

SemNode::SemNode(uint32_t value, uint32_t max)
    : m_count(value), m_max(max) {}

int SemNode::post() {
  m_lock.lock();
  if (m_count >= m_max) {
    m_lock.unlock();
    return -1;
  }
  ++m_count;
  m_lock.unlock();
  m_waiters.signal(1);
  return 0;
}

int SemNode::wait() {
  m_lock.lock();
  while (m_count == 0) {
    m_lock.unlock();
    m_waiters.wait();
    m_lock.lock();
  }
  --m_count;
  m_lock.unlock();
  return 0;
}

int SemNode::try_wait() {
  m_lock.lock();
  if (m_count == 0) {
    m_lock.unlock();
    return -1;
  }
  --m_count;
  m_lock.unlock();
  return 0;
}

int SemNode::get_value() const {
  m_lock.lock();
  uint32_t val = m_count;
  m_lock.unlock();
  return static_cast<int>(val);
}

}
