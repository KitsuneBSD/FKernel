#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Serial/serial_node.h>
#include <Kernel/Driver/Vga/vga_node.h>
#include <Kernel/Driver/Device/CharacterDevice/null_device.h>
#include <Kernel/Driver/Device/CharacterDevice/urandom_device.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Driver/Device/CharacterDevice/ptmx_device.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Fs/PtsFs/pts_dir_node.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/ProcFs/proc_fs.h>
#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

namespace {
struct MountRecord { char path[128]; char fstype[16]; uint32_t dev_id; };
static MountRecord s_mounts[64];
static size_t s_mount_count = 0;
static uint32_t s_next_dev_id = 1;
} // anonymous namespace

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
  (void)vfs.mount("/dev", fk::RefPtr<Node>(&devfs), "devfs");

  if (auto proc_res = fk::make_ref<ProcFsNode>()) {
    proc_res.value()->set_name("proc");
    (void)vfs.mount("/proc", proc_res.value(), "proc");
  }

  if (auto debugfs_res = fk::make_ref<fkernel::DebugFsNode>()) {
    debugfs_res.value()->set_name("debug");
    (void)vfs.mount("/debug", debugfs_res.value(), "debugfs");
  }

  auto& driver_manager = fkernel::DriverManager::the();
  driver_manager.register_device(fk::make_ref<fkernel::SerialNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ConsoleNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::CurrentTTYNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::NullDevice>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ZeroDevice>().value());
  driver_manager.register_device(fk::make_ref<fkernel::UrandomDevice>().value());

  driver_manager.register_device(fk::make_ref<fkernel::PtmxDevice>().value());

  // /dev/pts/ — PTY slave directory node (registered in DevFs so /dev/pts/N resolves)
  auto pts_node_res = fk::make_ref<PtsDirNode>();
  if (pts_node_res) {
    pts_node_res.value()->set_name("pts");
    fkernel::DevFs::the().register_device(pts_node_res.value(), "pts");
  }

  fkernel::terminal::TerminalManager::the().initialize();
  fk::algorithms::klog("VFS", "Initialized VFS with Dentry Cache");
}

VirtualFileSystem::VirtualFileSystem() : m_resolver(*this, m_lock) {}

VirtualFileSystem &VirtualFileSystem::the() {
  static VirtualFileSystem instance;
  return instance;
}

void VirtualFileSystem::mount_root(fk::RefPtr<Node> node) {
    if (!m_root) {
        m_root = Dentry::create("", nullptr).value();
    }
    m_root->push_node(node);
    if (s_mount_count < 64) {
        s_mounts[s_mount_count].path[0] = '/';
        s_mounts[s_mount_count].path[1] = '\0';
        fk::memory::copy_n(s_mounts[s_mount_count].fstype, "tmpfs", 15);
        s_mounts[s_mount_count].fstype[15] = '\0';
        s_mounts[s_mount_count].dev_id = s_next_dev_id++;
        ++s_mount_count;
    }
    fk::algorithms::klog("VFS", "Root mounted");
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::mount(const char *path, fk::RefPtr<Node> node, const char* fstype) {
  if (!path || !node) return fk::core::Error::InvalidParameter;
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  auto dentry = TRY(resolve_path_unlocked(path));
  dentry->push_node(node);

  if (s_mount_count < 64) {
    fk::memory::copy_n(s_mounts[s_mount_count].path, path, 127);
    s_mounts[s_mount_count].path[127] = '\0';
    fk::memory::copy_n(s_mounts[s_mount_count].fstype, fstype ? fstype : "auto", 15);
    s_mounts[s_mount_count].fstype[15] = '\0';
    s_mounts[s_mount_count].dev_id = s_next_dev_id++;
    ++s_mount_count;
  }

  fk::algorithms::klog("VFS", "Mounted node at %s (Stack size: %zu)", path, dentry->nodes().size());
  return {};
}

void VirtualFileSystem::for_each_mount(void (*cb)(const char* path, const char* fstype, void* ctx), void* ctx) {
  for (size_t i = 0; i < s_mount_count; ++i)
    cb(s_mounts[i].path, s_mounts[i].fstype, ctx);
}

uint32_t VirtualFileSystem::dev_id_for_path(const char* path) {
  if (!path) return 1;
  size_t best_len = 0;
  uint32_t best_dev = 1;
  for (size_t i = 0; i < s_mount_count; ++i) {
    const char* mp = s_mounts[i].path;
    size_t mp_len = __builtin_strlen(mp);
    if (__builtin_strncmp(path, mp, mp_len) == 0 && mp_len > best_len) {
      best_len = mp_len;
      best_dev = s_mounts[i].dev_id;
    }
  }
  return best_dev;
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::unmount(const char *path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry = TRY(resolve_path_unlocked(path));
  dentry->pop_node();

  for (size_t i = 0; i < s_mount_count; ++i) {
    if (__builtin_strcmp(s_mounts[i].path, path) == 0) {
      s_mounts[i] = s_mounts[--s_mount_count];
      break;
    }
  }

  fk::algorithms::klog("VFS", "unmounted %s", path);
  return {};
}

} // namespace fkernel
