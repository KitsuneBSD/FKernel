#include <Kernel/Fs/Virtual/MqueueFs/mqueue_node.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<MqueueNode>, fk::core::Error>
MqueueNode::create(size_t max_msgs, size_t max_msg_size) {
  if (max_msgs == 0) max_msgs = 10;
  if (max_msg_size == 0) max_msg_size = 8192;
  return fk::RefPtr<MqueueNode>(new MqueueNode(max_msgs, max_msg_size));
}

MqueueNode::MqueueNode(size_t max_msgs, size_t max_msg_size)
    : m_max_msgs(max_msgs), m_max_msg_size(max_msg_size) {}

int MqueueNode::send(const void* buf, size_t len, uint32_t prio) {
  if (len > m_max_msg_size) return -22;

  m_lock.lock();
  while (m_count >= m_max_msgs) {
    m_lock.unlock();
    m_endpoint.wait();
    m_lock.lock();
  }

  auto* entry = static_cast<Entry*>(kmalloc(sizeof(Entry)));
  if (!entry) {
    m_lock.unlock();
    return -12;
  }
  entry->prio = prio;
  entry->data.resize(len);
  fk::memory::copy(&entry->data[0], buf, len);

  m_queue.push_back(entry);
  ++m_count;
  m_lock.unlock();

  m_endpoint.signal(fk::NotificationBits(1));
  return 0;
}

ssize_t MqueueNode::receive(void* buf, size_t len, uint32_t* prio, bool nonblock) {
  m_lock.lock();
  while (m_count == 0) {
    m_lock.unlock();
    if (nonblock) return -1;
    m_endpoint.wait();
    m_lock.lock();
  }

  size_t best = 0;
  for (size_t i = 1; i < m_queue.size(); ++i) {
    if (m_queue[i]->prio > m_queue[best]->prio)
      best = i;
  }

  Entry* entry = m_queue[best];
  m_queue[best] = m_queue[m_queue.size() - 1];
  m_queue.pop_back();
  --m_count;
  m_lock.unlock();

  if (prio) *prio = entry->prio;
  size_t copy_len = (len < entry->data.size()) ? len : entry->data.size();
  fk::memory::copy(buf, &entry->data[0], copy_len);

  kfree(entry);
  m_endpoint.signal(fk::NotificationBits(1));
  return static_cast<ssize_t>(copy_len);
}

short MqueueNode::poll() const {
  m_lock.lock();
  bool has_data = m_count > 0;
  bool has_space = m_count < m_max_msgs;
  m_lock.unlock();
  short r = 0;
  if (has_data) r |= POLLIN;
  if (has_space) r |= POLLOUT;
  return r;
}

}
