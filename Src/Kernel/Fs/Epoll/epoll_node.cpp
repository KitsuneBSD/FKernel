#include <Kernel/Fs/Epoll/epoll_node.h>

bool EpollNode::ctl_add(int fd, uint32_t events, uint64_t data) {
  for (auto& e : m_entries) {
    if (e.fd == fd) return false; // already present
  }
  EpollEntry entry{fd, events, data};
  m_entries.push_back(entry);
  return true;
}

bool EpollNode::ctl_del(int fd) {
  for (size_t i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].fd == fd) {
      m_entries[i] = m_entries[m_entries.size() - 1];
      m_entries.pop_back();
      return true;
    }
  }
  return false;
}

bool EpollNode::ctl_mod(int fd, uint32_t events, uint64_t data) {
  for (auto& e : m_entries) {
    if (e.fd != fd) continue;
    e.events   = events;
    e.data_u64 = data;
    return true;
  }
  return false;
}
