#include <LibC/stdarg.h>
#include <LibC/stddef.h>
#include <LibC/string.h>
#include <Kernel/Driver/SerialPort/serial_node.h>
#include <Kernel/Driver/Vga/vga_node.h>
#include <Kernel/Driver/Device/CharacterDevice/null_device.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/ProcFs/proc_fs.h>
#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/Memory.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

void VirtualFileSystem::initialize() {
  auto &vfs = the();

  auto root_node = fk::make_ref<TmpFsDirectoryNode>().value();
  root_node->set_is_root(true);
  vfs.mount_root(root_node);

  (void)vfs.mkdir("/dev", 0755);
  (void)vfs.mkdir("/proc", 0755);
  (void)vfs.mkdir("/debug", 0755);
  (void)vfs.mkdir("/mnt", 0755);

  auto &devfs = fkernel::DevFs::the();
  devfs.set_name("dev");
  (void)vfs.mount("/dev", fk::RefPtr<Node>(&devfs));

  if (auto proc_res = fk::make_ref<ProcFsNode>()) {
    proc_res.value()->set_name("proc");
    (void)vfs.mount("/proc", proc_res.value());
  }

  if (auto debugfs_res = fk::make_ref<fkernel::DebugFsNode>()) {
    debugfs_res.value()->set_name("debug");
    (void)vfs.mount("/debug", debugfs_res.value());
  }

  auto& driver_manager = fkernel::DriverManager::the();
  driver_manager.register_device(fk::make_ref<fkernel::SerialNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ConsoleNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::CurrentTTYNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::NullDevice>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ZeroDevice>().value());

  fkernel::terminal::TerminalManager::the().initialize();
  fk::algorithms::klog("VFS", "Initialized VFS with Dentry Cache");
}

VirtualFileSystem &VirtualFileSystem::the() {
  static VirtualFileSystem instance;
  return instance;
}

void VirtualFileSystem::mount_root(fk::RefPtr<Node> node) {
    if (!m_root) {
        m_root = Dentry::create("", nullptr).value();
    }
    m_root->push_node(node);
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::mount(const char *path, fk::RefPtr<Node> node) {
  if (!path || !node) return fk::core::Error::InvalidParameter;
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  auto dentry = TRY(resolve_path_unlocked(path));
  dentry->push_node(node);
  
  fk::algorithms::klog("VFS", "Mounted node at %s (Stack size: %zu)", path, dentry->nodes().size());
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::unmount(const char *path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry = TRY(resolve_path_unlocked(path));
  
  dentry->pop_node();
  return {};
}

} // namespace fkernel
