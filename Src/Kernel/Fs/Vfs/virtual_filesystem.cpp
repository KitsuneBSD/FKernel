#include <LibC/stdarg.h>
#include <LibC/stddef.h>
#include <LibC/string.h>
#include <Kernel/Driver/SerialPort/serial_node.h>
#include <Kernel/Driver/Vga/vga_node.h>
#include <Kernel/Driver/Device/CharacterDevice/null_device.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
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

  // 1. Setup Root Dentry with TmpFs root node
  auto root_node = fk::make_ref<TmpFsDirectoryNode>().value();
  root_node->set_is_root(true);
  vfs.mount_root(root_node);

  // 2. Create base system directories in the TmpFs layer
  (void)vfs.mkdir("/dev", 0755);
  (void)vfs.mkdir("/proc", 0755);
  (void)vfs.mkdir("/debug", 0755);
  (void)vfs.mkdir("/mnt", 0755);

  // 3. Mount specialized filesystems
  auto &devfs = fkernel::DevFs::the();
  devfs.set_name("dev");
  (void)vfs.mount("/dev", fk::RefPtr<Node>(&devfs));

  auto proc_res = fk::make_ref<ProcFsNode>();
  if (proc_res) {
    proc_res.value()->set_name("proc");
    (void)vfs.mount("/proc", proc_res.value());
  }

  auto debugfs_res = fk::make_ref<fkernel::DebugFsNode>();
  if (debugfs_res) {
    debugfs_res.value()->set_name("debug");
    (void)vfs.mount("/debug", debugfs_res.value());
  }

  // 4. Initialize Core Drivers
  auto& driver_manager = fkernel::DriverManager::the();
  driver_manager.register_device(fk::make_ref<fkernel::SerialNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ConsoleNode>().value());
  driver_manager.register_device(fk::make_ref<fkernel::NullDevice>().value());
  driver_manager.register_device(fk::make_ref<fkernel::ZeroDevice>().value());

  // 5. Initialize Terminal System
  vfs.symlink("/dev/tty", "tty0");
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

  auto dentry_res = resolve_path(path);
  if (dentry_res.is_error()) return dentry_res.error();

  auto dentry = dentry_res.value();
  dentry->push_node(node);
  
  fk::algorithms::klog("VFS", "Mounted node at %s (Stack size: %zu)", path, dentry->nodes().size());
  return {};
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
VirtualFileSystem::resolve_path(const char *path, fk::RefPtr<Dentry> base, int depth) {
  if (depth > 8) return fk::core::Error::IOError;
  if (!path || !m_root) return fk::core::Error::InvalidParameter;

  fk::RefPtr<Dentry> current = m_root;
  const char *ptr = path;

  if (path[0] == '/') {
      // Absolute path, start from root
      while (*ptr == '/') ptr++;
  } else if (base) {
      current = base;
  } else {
      // Relative path, start from CWD
      auto *task = SchedulerManager::the().current();
      if (task && !task->cwd.empty()) {
          auto cwd_res = resolve_path(task->cwd.c_str(), nullptr, depth + 1);
          if (cwd_res.is_ok()) current = cwd_res.value();
      }
  }

  while (*ptr) {
    char name[256];
    size_t i = 0;
    while (*ptr && *ptr != '/' && i < 255) name[i++] = *ptr++;
    name[i] = '\0';

    if (name[0] == '\0' || strcmp(name, ".") == 0) {
        while (*ptr == '/') ptr++;
        continue;
    }

    auto next_res = current->lookup(name);
    if (next_res.is_error()) return next_res.error();
    current = next_res.value();

    // Handle Symlinks
    int symlink_depth = 0;
    while (current->top_node() && current->top_node()->is_symlink() && symlink_depth < 8) {
        auto link_res = current->top_node()->read_link();
        if (link_res.is_error()) return link_res.error();
        
        fk::text::String link = link_res.value();
        if (link.c_str()[0] == '/') {
            auto sub_res = resolve_path(link.c_str(), nullptr, depth + 1);
            if (sub_res.is_error()) return sub_res.error();
            current = sub_res.value();
        } else {
            // Resolve relative to parent dentry
            auto sub_res = resolve_path(link.c_str(), current->parent(), depth + 1);
            if (sub_res.is_error()) return sub_res.error();
            current = sub_res.value();
        }
        symlink_depth++;
    }

    while (*ptr == '/') ptr++;
  }

  return current;
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
VirtualFileSystem::resolve_path_to_parent(const char *path, int depth) {
    if (!path || path[0] == '\0') return fk::core::Error::InvalidParameter;

    char parent_path[512];
    strncpy(parent_path, path, 511);
    parent_path[511] = '\0';

    char *last_slash = strrchr(parent_path, '/', 512);
    fk::text::String name;

    if (!last_slash) {
        name = path;
        auto *task = SchedulerManager::the().current();
        auto cwd_res = resolve_path(task ? task->cwd.c_str() : "/", nullptr, depth + 1);
        if (cwd_res.is_error()) return cwd_res.error();
        return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(cwd_res.value(), name);
    }

    if (last_slash == parent_path) {
        name = last_slash + 1;
        return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(m_root, name);
    }

    *last_slash = '\0';
    name = last_slash + 1;
    auto parent_res = resolve_path(parent_path, nullptr, depth + 1);
    if (parent_res.is_error()) return parent_res.error();
    return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(parent_res.value(), name);
}

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char *path, int flags) {
  auto dentry_res = resolve_path(path);
  if (dentry_res.is_error()) return dentry_res.error();
  
  auto dentry = dentry_res.value();
  auto node = dentry->top_node();
  
  if (!node) return fk::core::Error::NotFound;
  if ((flags & O_DIRECTORY) && !node->is_directory()) return fk::core::Error::NotADirectory;
  
  return fk::make_ref<FileDescription>(dentry, flags).value();
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::mkdir(const char *path, int mode) {
  auto parent_res = resolve_path_to_parent(path);
  if (parent_res.is_error()) return parent_res.error();
  
  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node) return fk::core::Error::NotFound;

  TRY(node->mkdir(name.c_str(), mode));
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::symlink(const char *path, const char *target) {
  auto parent_res = resolve_path_to_parent(path);
  if (parent_res.is_error()) return parent_res.error();
  
  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node) return fk::core::Error::NotFound;

  TRY(node->symlink(name.c_str(), target));
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::readdir(const char *path, fk::containers::Vector<DirectoryEntry> &entries) {
  auto dentry_res = resolve_path(path);
  if (dentry_res.is_error()) return dentry_res.error();
  
  auto dentry = dentry_res.value();
  
  // Logical Union View: Merge entries from ALL nodes in the stack
  for (auto& node : dentry->nodes()) {
      fk::containers::Vector<DirectoryEntry> layer_entries;
      (void)node->list_dir(layer_entries);
      for (auto& entry : layer_entries) {
          add_directory_entry(entries, entry);
      }
  }
  
  // Also add cached children (which might represent sub-mounts)
  for (auto& child : dentry->children()) {
      DirectoryEntry de;
      strncpy(de.name, child->name().c_str(), sizeof(de.name) - 1);
      de.type = child->top_node()->is_directory() ? 1 : 0;
      add_directory_entry(entries, de);
  }

  return {};
}

fk::core::Result<size_t, fk::core::Error>
VirtualFileSystem::readdir(fk::RefPtr<FileDescription> description, uint8_t *buffer, size_t max_bytes) {
  if (!buffer) return fk::core::Error::InvalidParameter;

  fk::containers::Vector<DirectoryEntry> entries;
  DirectoryEntry dot; strcpy(dot.name, "."); dot.type = 1; add_directory_entry(entries, dot);
  DirectoryEntry dotdot; strcpy(dotdot.name, ".."); dotdot.type = 1; add_directory_entry(entries, dotdot);

  auto dentry = description->dentry();
  for (auto& node : dentry->nodes()) {
      fk::containers::Vector<DirectoryEntry> layer_entries;
      (void)node->list_dir(layer_entries);
      for (auto& entry : layer_entries) {
          add_directory_entry(entries, entry);
      }
  }

  for (auto& child : dentry->children()) {
      DirectoryEntry de;
      strncpy(de.name, child->name().c_str(), sizeof(de.name) - 1);
      de.type = child->top_node() && child->top_node()->is_directory() ? 1 : 0;
      add_directory_entry(entries, de);
  }

  uint64_t bytes_written = 0;
  uint64_t start_idx = description->offset();

  for (size_t i = start_idx; i < entries.size(); ++i) {
    auto &entry = entries[i];
    size_t name_len = strlen(entry.name);
    size_t reclen = (offsetof(linux_dirent64, d_name) + name_len + 1 + 7) & ~7;

    if (bytes_written + reclen > max_bytes) break;

    auto *dirent = reinterpret_cast<linux_dirent64 *>(buffer + bytes_written);
    dirent->d_ino = i + 1; dirent->d_off = i + 1; dirent->d_reclen = static_cast<uint16_t>(reclen);
    uint8_t dt_type = DT_UNKNOWN;
    if (entry.type == 0) dt_type = DT_REG;
    else if (entry.type == 1) dt_type = DT_DIR;
    else if (entry.type == 2) dt_type = DT_LNK;
    dirent->d_type = dt_type;
    fk::memory::copy(dirent->d_name, entry.name, name_len);
    dirent->d_name[name_len] = '\0';

    bytes_written += reclen;
    description->set_offset(i + 1);
  }

  return bytes_written;
}

void VirtualFileSystem::add_directory_entry(fk::containers::Vector<DirectoryEntry>& entries, const DirectoryEntry& entry) {
    for (auto& existing : entries) {
        if (strcmp(existing.name, entry.name) == 0) return;
    }
    entries.push_back(entry);
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::stat(const char *path, struct stat *buf) {
  auto dentry_res = resolve_path(path);
  if (dentry_res.is_error()) return dentry_res.error();
  
  auto node = dentry_res.value()->top_node();
  if (!node) return fk::core::Error::NotFound;

  fk::memory::set(buf, 0, sizeof(struct stat));
  buf->st_dev = 1;
  buf->st_size = node->size();
  buf->st_blksize = 4096;
  buf->st_blocks = (buf->st_size + 511) / 512;
  buf->st_ino = reinterpret_cast<uintptr_t>(node.get());
  buf->st_atime = buf->st_mtime = buf->st_ctime = 1000000;

  if (node->is_directory()) {
    buf->st_mode = S_IFDIR | 0755;
    buf->st_nlink = 2;
    if (buf->st_size == 0) buf->st_size = 4096;
  } else if (node->is_symlink()) {
    buf->st_mode = S_IFLNK | 0777;
    buf->st_nlink = 1;
  } else if (node->is_character_device()) {
    buf->st_mode = S_IFCHR | 0666;
    buf->st_nlink = 1;
  } else if (node->is_block_device()) {
    buf->st_mode = S_IFBLK | 0660;
    buf->st_nlink = 1;
  } else {
    buf->st_mode = S_IFREG | 0644;
    buf->st_nlink = 1;
  }

  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::unmount(const char *path) {
  auto dentry_res = resolve_path(path);
  if (dentry_res.is_error()) return dentry_res.error();
  
  dentry_res.value()->pop_node();
  return {};
}

} // namespace fkernel