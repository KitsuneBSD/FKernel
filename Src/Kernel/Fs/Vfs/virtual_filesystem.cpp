#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Serial/serial_node.h>
#include <Kernel/Driver/Vga/vga_node.h>
#include <Kernel/Driver/Device/CharacterDevice/null_device.h>
#include <Kernel/Driver/Device/CharacterDevice/urandom_device.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/Virtual/DebugFs/debug_fs.h>
#include <Kernel/Driver/Device/CharacterDevice/ptmx_device.h>
#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>
#include <Kernel/Fs/Virtual/DevFs/tty.h>
#include <Kernel/Fs/Virtual/PtsFs/pts_dir_node.h>
#include <Kernel/Fs/Virtual/SemFs/sem_dir_node.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_dir_node.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_dir_node.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/Virtual/ProcFs/proc_fs.h>
#include <Kernel/Fs/Virtual/SysFs/sys_fs.h>
#include <Kernel/Fs/Virtual/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/mount_namespace.h>
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
  (void)vfs.mkdir("/sys", 0755);
  (void)vfs.mkdir("/debug", 0755);
  (void)vfs.mkdir("/mnt", 0755);
  (void)vfs.mkdir("/tmp", 0755);

  auto &devfs = fkernel::DevFs::the();
  devfs.set_name("dev");
  (void)vfs.mount("/dev", fk::RefPtr<Node>(&devfs), "devfs");

  if (auto proc_res = fk::make_ref<ProcFsNode>()) {
    proc_res.value()->set_name("proc");
    (void)vfs.mount("/proc", proc_res.value(), "proc");
  }

  if (auto sysfs_res = fk::make_ref<SysFsNode>()) {
    sysfs_res.value()->set_name("sys");
    (void)vfs.mount("/sys", sysfs_res.value(), "sysfs");
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

  auto sem_node_res = fk::make_ref<SemDirNode>();
  if (sem_node_res) {
    sem_node_res.value()->set_name("sem");
    fkernel::DevFs::the().register_device(sem_node_res.value(), "sem");
  }

  auto mqueue_node_res = fk::make_ref<MqueueDirNode>();
  if (mqueue_node_res) {
    mqueue_node_res.value()->set_name("mqueue");
    fkernel::DevFs::the().register_device(mqueue_node_res.value(), "mqueue");
  }

  auto shm_node_res = fk::make_ref<ShmDirNode>();
  if (shm_node_res) {
    shm_node_res.value()->set_name("shm");
    fkernel::DevFs::the().register_device(shm_node_res.value(), "shm");
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
  auto* ns = current_mount_namespace();
  if (ns) {
    ns->push_mount(dentry.get(), node, path, fstype);
    fk::algorithms::klog("VFS", "Mounted node at %s (NS stack)", path);
    return {};
  }

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
  auto* ns = current_mount_namespace();
  if (ns) {
    for (size_t i = 0; i < ns->mount_count(); ++i)
      cb(ns->mounts()[i].path, ns->mounts()[i].fstype, ctx);
    return;
  }
  for (size_t i = 0; i < s_mount_count; ++i)
    cb(s_mounts[i].path, s_mounts[i].fstype, ctx);
}

uint32_t VirtualFileSystem::dev_id_for_path(const char* path) {
  auto* ns = current_mount_namespace();
  if (ns) return ns->dev_id_for_path(path);
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

  auto* ns = current_mount_namespace();
  if (ns) {
    ns->pop_mount(path);
    auto* ns_stack = ns->get_stack(dentry.get());
    if (ns_stack) ns_stack->pop();
    fk::algorithms::klog("VFS", "unmounted %s (NS)", path);
    return {};
  }

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

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::pivot_root(const char* new_root_path, const char* put_old_path) {
  if (!new_root_path || !put_old_path) return fk::core::Error::InvalidParameter;
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  auto new_root_dentry = TRY(resolve_path_unlocked(new_root_path));
  auto put_old_dentry = TRY(resolve_path_unlocked(put_old_path));

  if (new_root_dentry->nodes().is_empty())
    return fk::core::Error::InvalidParameter;

  auto* put_old_node = put_old_dentry->top_node().get();
  if (!put_old_node || !put_old_node->is_directory())
    return fk::core::Error::NotADirectory;

  auto* target = put_old_dentry.get();
  while (target) {
    if (target == new_root_dentry.get()) break;
    target = target->parent().get();
  }
  if (!target) return fk::core::Error::InvalidParameter;

  auto root_dentry = m_root;
  if (!root_dentry) return fk::core::Error::IOError;
  if (root_dentry.get() == new_root_dentry.get())
    return fk::core::Error::InvalidParameter;

  auto old_root_node = root_dentry->top_node();
  if (!old_root_node) return fk::core::Error::IOError;
  auto new_root_fs_node = new_root_dentry->top_node();

  root_dentry->pop_node();
  new_root_dentry->pop_node();

  root_dentry->push_node(new_root_fs_node);
  put_old_dentry->push_node(old_root_node);

  auto* ns = current_mount_namespace();
  if (ns) {
    char new_root_fstype[16] = "rootfs";
    const auto* ns_mounts = ns->mounts();
    for (size_t i = 0; i < ns->mount_count(); ++i) {
      if (__builtin_strcmp(ns_mounts[i].path, new_root_path) == 0) {
        fk::memory::copy_n(new_root_fstype, ns_mounts[i].fstype, 15);
        new_root_fstype[15] = '\0';
        break;
      }
    }
    ns->pop_mount(new_root_path);
    ns->update_root_mount(new_root_fstype);
    ns->add_mount_entry(put_old_path, "rootfs", put_old_dentry.get());
  } else {
    char new_root_fstype[16] = "rootfs";
    for (size_t i = 0; i < s_mount_count; ++i) {
      if (__builtin_strcmp(s_mounts[i].path, new_root_path) == 0) {
        fk::memory::copy_n(new_root_fstype, s_mounts[i].fstype, 15);
        new_root_fstype[15] = '\0';
        s_mounts[i] = s_mounts[--s_mount_count];
        break;
      }
    }

    size_t write = 0;
    for (size_t i = 0; i < s_mount_count; ++i) {
      if (__builtin_strcmp(s_mounts[i].path, "/") != 0)
        s_mounts[write++] = s_mounts[i];
    }
    s_mount_count = write;

    if (s_mount_count < 64) {
      fk::memory::copy_n(s_mounts[s_mount_count].path, "/", 2);
      fk::memory::copy_n(s_mounts[s_mount_count].fstype, new_root_fstype, 15);
      s_mounts[s_mount_count].fstype[15] = '\0';
      s_mounts[s_mount_count].dev_id = s_next_dev_id++;
      ++s_mount_count;
    }

    if (s_mount_count < 64) {
      fk::memory::copy_n(s_mounts[s_mount_count].path, put_old_path, 127);
      s_mounts[s_mount_count].path[127] = '\0';
      fk::memory::copy_n(s_mounts[s_mount_count].fstype, "rootfs", 15);
      s_mounts[s_mount_count].fstype[15] = '\0';
      s_mounts[s_mount_count].dev_id = s_next_dev_id++;
      ++s_mount_count;
    }
  }

  fk::algorithms::klog("VFS", "pivot_root: %s -> /, old root -> %s", new_root_path, put_old_path);
  return {};
}

fk::RefPtr<MountNamespace> VirtualFileSystem::clone_mount_namespace() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  auto* existing_ns = current_mount_namespace();
  fk::RefPtr<Dentry> ns_root = m_root;

  auto new_ns = MountNamespace::create(ns_root);

  if (existing_ns) {
    for (size_t i = 0; i < existing_ns->mount_count(); ++i) {
      auto& m = existing_ns->mounts()[i];
      auto dentry_res = resolve_path_unlocked(m.path);
      if (dentry_res.is_error()) continue;
      auto* d = dentry_res.value().get();
      auto* stack = existing_ns->get_stack(d);
      if (!stack) continue;
      auto& child_stack = new_ns->ensure_stack(d);
      const auto& nodes = stack->all();
      for (size_t n = 0; n < nodes.size(); ++n)
        child_stack.push(nodes[n]);
      new_ns->add_mount_entry(m.path, m.fstype, d);
    }
  } else {
    for (size_t i = 0; i < s_mount_count; ++i) {
      auto dentry_res = resolve_path_unlocked(s_mounts[i].path);
      if (dentry_res.is_error()) continue;
      auto* d = dentry_res.value().get();
      auto& child_stack = new_ns->ensure_stack(d);
      const auto& def_nodes = d->default_stack().all();
      for (size_t n = 0; n < def_nodes.size(); ++n)
        child_stack.push(def_nodes[n]);
      new_ns->add_mount_entry(s_mounts[i].path, s_mounts[i].fstype, d);
    }
  }

  return new_ns;
}

} // namespace fkernel
