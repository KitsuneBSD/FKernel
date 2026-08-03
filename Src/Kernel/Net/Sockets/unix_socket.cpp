#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Net/Sockets/unix_socket.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<UnixSocket>, fk::core::Error> UnixSocket::create(SocketType type) {
  return fk::make_ref<UnixSocket>(type);
}

fk::core::Result<void, fk::core::Error> UnixSocket::create_pair(
    SocketType type, fk::RefPtr<UnixSocket>& a, fk::RefPtr<UnixSocket>& b) {
  auto ra = fk::make_ref<UnixSocket>(type);
  if (ra.is_error()) return ra.error();
  auto rb = fk::make_ref<UnixSocket>(type);
  if (rb.is_error()) return rb.error();
  auto* sa = ra.value().get();
  auto* sb = rb.value().get();
  sa->m_peer      = sb;
  sa->m_connected = true;
  sb->m_peer      = sa;
  sb->m_connected = true;
  a = ra.value();
  b = rb.value();
  return {};
}

UnixSocket::UnixSocket(SocketType type) : m_type(type) {}

UnixSocket::~UnixSocket() = default;

fk::core::Result<void, fk::core::Error> UnixSocket::bind(const char* path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto res = VirtualFileSystem::the().mount(path, fk::RefPtr<Node>(this));
  if (res.is_ok()) {
    m_bound_path = fk::text::fixed_string<108>(path);
    fk::algorithms::kdebug("UNIX", "bind(%s)", path);
  }
  return res;
}

fk::core::Result<void, fk::core::Error> UnixSocket::connect(const char* path) {
  fk::algorithms::kdebug("UNIX", "connect(%s)", path);
  auto dentry_res = VirtualFileSystem::the().resolve_path(path);
  if (dentry_res.is_error()) {
    fk::algorithms::kwarn("UNIX", "connect failed: %s", path);
    return dentry_res.error();
  }

  auto node = dentry_res.value()->top_node();
  if (!node) {
    fk::algorithms::kwarn("UNIX", "connect failed: %s", path);
    return fk::core::Error::NotFound;
  }

  auto peer = static_cast<UnixSocket*>(node.get());

  fk::synchronization::ScopedLockIRQ lock(peer->m_lock);
  if (!peer->m_listening) {
    fk::algorithms::kwarn("UNIX", "connect failed: %s", path);
    return fk::core::Error::PermissionDenied;
  }

  if (peer->m_backlog_count >= 16) {
    fk::algorithms::kwarn("UNIX", "connect failed: %s", path);
    return fk::core::Error::DeviceError;
  }

  peer->m_backlog[peer->m_backlog_count++] = fk::RefPtr<UnixSocket>(this);
  m_peer = peer;
  m_connected = true;

  // Capture caller's credentials for SO_PEERCRED / SCM_CREDENTIALS.
  auto* current = SchedulerManager::the().current();
  if (current) {
    m_peer_creds.pid = current->control.identity.id.value();
    m_peer_creds.uid = current->control.identity.uid;
    m_peer_creds.gid = current->control.identity.gid;
  }

  peer->m_accept_endpoint.signal(fk::NotificationBits(1));

  return {};
}

fk::core::Result<void, fk::core::Error> UnixSocket::listen() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  m_listening = true;
  return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> UnixSocket::accept() {
  while (true) {
    {
      fk::synchronization::ScopedLockIRQ lock(m_lock);
      if (m_backlog_count > 0) {
        auto client = m_backlog[0];
        for (size_t i = 0; i < m_backlog_count - 1; i++)
          m_backlog[i] = m_backlog[i + 1];
        m_backlog_count--;
        return fk::RefPtr<Socket>(client.get());
      }
    }
    TRY(m_accept_endpoint.wait_interruptible());
  }
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::read(uint64_t, size_t size, uint8_t* buffer) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  size_t n = m_rx_buffer.read(buffer, size);
  if (n > 0 && m_peer)
    notify_kqueue_writers(m_peer);
  return n;
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::write(uint64_t, size_t size,
                                                            const uint8_t* buffer) {
  if (!m_peer)
    return fk::core::Error::IOError;

  fk::synchronization::ScopedLockIRQ lock(m_peer->m_lock);
  size_t n = m_peer->m_rx_buffer.write(buffer, size);
  if (n > 0)
    notify_kqueue_readers(m_peer);
  return n;
}

fk::core::Result<void, fk::core::Error> UnixSocket::getsockname(char* addr, uint32_t* addrlen) {
  if (!addr || !addrlen) return fk::core::Error::InvalidParameter;
  // sockaddr_un: sa_family(2) + sun_path(108)
  *reinterpret_cast<uint16_t*>(addr) = 1; // AF_UNIX
  char* sun_path = addr + 2;
  const char* path = m_bound_path.buffer;
  size_t i = 0;
  while (path[i] && i < 107) { sun_path[i] = path[i]; i++; }
  sun_path[i] = '\0';
  *addrlen = (uint32_t)(2 + i + 1);
  return {};
}

fk::core::Result<void, fk::core::Error> UnixSocket::getpeername(char* addr, uint32_t* addrlen) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  if (!m_peer) return fk::core::Error::InvalidParameter;
  return m_peer->getsockname(addr, addrlen);
}

// SO_PEERCRED returns the credentials of the socket that called connect() (i.e. m_peer_creds).
// For the server side, getsockopt is called on the accepted client socket, so m_peer_creds holds
// the connecting process's pid/uid/gid — exactly what SO_PEERCRED should return.
fk::core::Result<void, fk::core::Error> UnixSocket::getsockopt(
    int level, int optname, void* optval, uint32_t* optlen) {
  static constexpr int SOL_SOCKET = 1;
  static constexpr int SO_PEERCRED = 17;
  if (level != SOL_SOCKET || optname != SO_PEERCRED)
    return fk::core::Error::ProtocolNotSupported;
  if (!optval || !optlen)
    return fk::core::Error::InvalidParameter;
  if (*optlen < sizeof(PeerCredentials))
    return fk::core::Error::InvalidParameter;
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  PeerCredentials creds = m_peer_creds;
  fk::memory::copy(optval, &creds, sizeof(PeerCredentials));
  *optlen = sizeof(PeerCredentials);
  return {};
}

void UnixSocket::send_fds(fk::containers::Vector<fk::RefPtr<FileDescription>>& fds) {
  if (!m_peer) return;
  fk::synchronization::ScopedLockIRQ lock(m_peer->m_lock);
  for (size_t i = 0; i < fds.size(); ++i) {
    if (m_peer->m_pending_fd_count >= MAX_PENDING_FDS) break;
    m_peer->m_pending_fds[m_peer->m_pending_fd_count++] = fds[i];
  }
}

void UnixSocket::recv_fds(fk::containers::Vector<fk::RefPtr<FileDescription>>& out) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  for (size_t i = 0; i < m_pending_fd_count; ++i)
    out.push_back(fk::types::move(m_pending_fds[i]));
  m_pending_fd_count = 0;
}

} // namespace fkernel
