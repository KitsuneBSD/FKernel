#include <Kernel/Net/unix_socket.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<UnixSocket>, fk::core::Error> UnixSocket::create(SocketType type) {
    return fk::make_ref<UnixSocket>(type);
}

UnixSocket::UnixSocket(SocketType type) : m_type(type) {
    // Allocate buffer for communication (1 page)
    uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
    MemoryManager::the().map_page(phys, phys, PageFlags::Present | PageFlags::Writable);
    m_buffer = reinterpret_cast<uint8_t*>(phys);
    m_buffer_size = 4096;
}

UnixSocket::~UnixSocket() {
    if (m_buffer) {
        // In a real kernel, we'd unmap and free
    }
}

fk::core::Result<void, fk::core::Error> UnixSocket::bind(const char* path) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    fk::algorithms::klog("UnixSocket", "Binding socket to %s", path);
    
    // In a real implementation, we would create a special node in the VFS
    // For now, let's just mount this node to the path
    return VirtualFileSystem::the().mount(path, fk::RefPtr<Node>(this));
}

fk::core::Result<void, fk::core::Error> UnixSocket::connect(const char* path) {
    fk::algorithms::klog("UnixSocket", "Connecting from %p to %s", this, path);
    
    auto dentry_res = VirtualFileSystem::the().resolve_path(path);
    if (dentry_res.is_error()) {
        fk::algorithms::kerror("UnixSocket", "Connection failed: path %s not found", path);
        return dentry_res.error();
    }

    auto node = dentry_res.value()->top_node();
    if (!node) return fk::core::Error::NotFound;

    auto peer = static_cast<UnixSocket*>(node.get());
    
    fk::synchronization::ScopedLockIRQ lock(peer->m_lock);
    if (!peer->m_listening) {
        fk::algorithms::kwarn("UnixSocket", "Connection refused: %s is not listening", path);
        return fk::core::Error::PermissionDenied;
    }

    if (peer->m_backlog_count >= 16) {
        fk::algorithms::kwarn("UnixSocket", "Connection dropped: backlog full for %s", path);
        return fk::core::Error::DeviceError;
    }

    peer->m_backlog[peer->m_backlog_count++] = fk::RefPtr<UnixSocket>(this);
    m_peer = fk::RefPtr<UnixSocket>(peer);
    m_connected = true;

    fk::algorithms::klog("UnixSocket", "Connection request queued for %s", path);
    return {};
}

fk::core::Result<void, fk::core::Error> UnixSocket::listen() {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    m_listening = true;
    fk::algorithms::klog("UnixSocket", "Socket %p is now listening", this);
    return {};
}

fk::core::Result<fk::RefPtr<Socket>, fk::core::Error> UnixSocket::accept() {
    fk::algorithms::klog("UnixSocket", "Socket %p waiting for connection...", this);
    while (true) {
        {
            fk::synchronization::ScopedLockIRQ lock(m_lock);
            if (m_backlog_count > 0) {
                auto client = m_backlog[0];
                for (size_t i = 0; i < m_backlog_count - 1; i++) m_backlog[i] = m_backlog[i+1];
                m_backlog_count--;
                fk::algorithms::klog("UnixSocket", "Socket %p accepted connection from %p", this, client.get());
                return fk::RefPtr<Socket>(client.get());
            }
        }
        asm volatile("pause");
    }
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::read(uint64_t, size_t size, uint8_t* buffer) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    
    size_t available = m_write_ptr - m_read_ptr;
    if (available == 0) return (size_t)0;

    size_t to_read = size < available ? size : available;
    memcpy(buffer, m_buffer + (m_read_ptr % m_buffer_size), to_read);
    m_read_ptr += to_read;

    fk::algorithms::kdebug("UnixSocket", "Read %zu bytes from socket %p", to_read, this);
    return to_read;
}

fk::core::Result<size_t, fk::core::Error> UnixSocket::write(uint64_t, size_t size, const uint8_t* buffer) {
    if (!m_peer) {
        fk::algorithms::kerror("UnixSocket", "Write failed: socket %p not connected", this);
        return fk::core::Error::IOError;
    }

    fk::synchronization::ScopedLockIRQ lock(m_peer->m_lock);
    
    size_t available = m_peer->m_buffer_size - (m_peer->m_write_ptr - m_peer->m_read_ptr);
    if (available == 0) return (size_t)0;

    size_t to_write = size < available ? size : available;
    memcpy(m_peer->m_buffer + (m_peer->m_write_ptr % m_peer->m_buffer_size), buffer, to_write);
    m_peer->m_write_ptr += to_write;

    fk::algorithms::kdebug("UnixSocket", "Wrote %zu bytes to socket %p (peer %p)", to_write, this, m_peer.get());
    return to_write;
}

} // namespace fkernel
