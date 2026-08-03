// Kernel VFS stubs for host-side unit tests.
// current_mount_namespace() → nullptr: Dentry operations use the per-Dentry
// default node stack and the built-in child-map cache.
// MountNamespace::get_stack/ensure_stack are unreachable via the nullptr
// guard in dentry.cpp, but the linker still needs them defined.

#include <Kernel/Fs/Vfs/Core/dentry_node_stack.h>

namespace fkernel {

class MountNamespace;
class Dentry;

MountNamespace* current_mount_namespace() { return nullptr; }

} // namespace fkernel

// MountNamespace method stubs — never called because current_mount_namespace()
// always returns nullptr.  Defined here to satisfy the linker.
#include <Kernel/Fs/Vfs/Mount/mount_namespace.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>

namespace fkernel {

DentryNodeStack* MountNamespace::get_stack(Dentry*) { return nullptr; }
DentryNodeStack& MountNamespace::ensure_stack(Dentry* d) {
    return d->default_stack();
}

} // namespace fkernel
