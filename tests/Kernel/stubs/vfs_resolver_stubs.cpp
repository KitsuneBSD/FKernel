// Minimal VirtualFileSystem and IPC stubs for host-side VFS unit tests.
//
// file_description.cpp's destructor references PipeNode::remove_reader() (inline),
// which calls Endpoint::signal().  MockFileNode::is_pipe() always returns false
// so the branch is never taken at runtime, but the linker still needs the symbol.
//
// PathResolver only calls m_vfs.root() (inline) and nothing else on the VFS.
// We provide the singleton constructor, the() factory, and mount_root() so
// tests can plant a known Dentry tree before exercising the resolver.
//
// All other VirtualFileSystem methods are NOT defined here; the linker will
// complain only if test code actually calls them.

#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Ipc/Endpoints/endpoint.h>

namespace fkernel {

VirtualFileSystem::VirtualFileSystem()
    : m_lock(), m_resolver(*this, m_lock), m_root() {}

VirtualFileSystem& VirtualFileSystem::the() {
    static VirtualFileSystem inst;
    return inst;
}

void VirtualFileSystem::mount_root(fk::RefPtr<Node> node) {
    if (!m_root) {
        auto r = Dentry::create("", nullptr);
        if (r.is_ok()) m_root = r.value();
    }
    if (m_root && node) m_root->push_node(node);
}

} // namespace fkernel

namespace fkernel { namespace ipc {
void Endpoint::signal(fk::NotificationBits) {}
}} // namespace fkernel::ipc
