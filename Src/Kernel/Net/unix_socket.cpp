#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Net/unix_socket.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<UnixSocket>, fk::core::Error> UnixSocket::create(SocketType type) {
  return fk::make_ref<UnixSocket>(type);
}

UnixSocket::UnixSocket(SocketType type) : m_type(type) {}

UnixSocket::~UnixSocket() = default;

fk::core::Result<void, fk::core::Error> UnixSocket::bind(const char* path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  // In a real implementation, we would create a special node in the VFS
  // For now, let's just mount this node to the path
  return VirtualFileSystem::the().mount(path, fk::RefPtr<Node>(this));
}

fk::core::Result<void, fk::core::Error> UnixSocket::connect(const char* path) {
  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) {
    return dentry_res.error();
  }

  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  auto peer = static_cast<UnixSocket*>(node.get());

  fk::synchronization::ScopedLockIRQ lock(peer->m_lock);
  if (!peer->m_listening) {
    return fk::core::Error::PermissionDenied;
  }

  if (peer->m_backlog_count >= 16) {
    return fk::core::Error::DeviceError;
  }

  peer->m_backlog[peer->m_backlog_count++] = fk::RefPtr<UnixSocket>(this);
  m_peer = fk::RefPtr<UnixSocket>(peer);
  m_connected = true;

  // Wake any task blocked in accept()
  if (peer->m_accept_waiter) {
    SchedulerManager::the().wake_task(peer->m_accept_waiter);
    peer->m_accept_waiter = nullptr;
  }

  return {};
}

fk::core::Result<void, fk::core::Error> UnixSocket::listen() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_listening = true;
  return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> UnixSocket::accept() {
  auto& scheduler = SchedulerManager::the();
  while (true) {
    {
      fk::synchronization::ScopedLockIRQ lock(m_lock);
      if (m_backlog_count > 0) {
        auto client = m_backlog[0];
        for (size_t i = 0; i < m_backlog_count - 1; i++)
          m_backlog[i] = m_backlog[i + 1];
        m_backlog_count--;
        m_accept_waiter = nullptr;
        return fk::RefPtr<Socket>(client.get());
      }
      m_accept_waiter = scheduler.current();
    }
    scheduler.block_current();
  }
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::read(uint64_t, size_t size, uint8_t* buffer) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return m_rx_buffer.read(buffer, size);
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::write(uint64_t, size_t size,
                                                            const uint8_t* buffer) {
  if (!m_peer)
    return fk::core::Error::IOError;

  fk::synchronization::ScopedLockIRQ lock(m_peer->m_lock);
  return m_peer->m_rx_buffer.write(buffer, size);
}

} // namespace fkernel
