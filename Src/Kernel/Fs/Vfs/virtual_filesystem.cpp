#include <Kernel/Driver/SerialPort/serial_node.h>
#include <Kernel/Driver/Vga/vga_node.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Fs/ProcFs/proc_fs.h>
#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/Memory.h>

void VirtualFileSystem::initialize() {
  auto &vfs = the();

  auto root_node = fk::make_ref<TmpFsDirectoryNode>().value();
  root_node->set_is_root(true);
  vfs.mount_root(root_node);

  root_node->mkdir("dev", 0755);
  root_node->mkdir("proc", 0755);
  root_node->mkdir("debug", 0755);
  root_node->mkdir("mnt", 0755);

  auto &devfs = fkernel::DevFs::the();
  vfs.mount("/dev", fk::RefPtr<Node>(&devfs));

  devfs.register_device(fk::make_ref<fkernel::TtyNode>().value(), "console");
  devfs.register_device(fk::make_ref<fkernel::SerialNode>().value(), "serial");
  devfs.register_device(fk::make_ref<fkernel::TtyNode>().value(), "tty0");

  auto proc_res = fk::make_ref<ProcFsNode>();
  if (proc_res)
    vfs.mount("/proc", proc_res.value());

  auto debugfs_res = fk::make_ref<fkernel::DebugFsNode>();
  if (debugfs_res)
    vfs.mount("/debug", debugfs_res.value());

  fk::algorithms::klog("VFS", "Initialized VFS");
}

VirtualFileSystem &VirtualFileSystem::the() {
  static VirtualFileSystem instance;
  return instance;
}

void VirtualFileSystem::mount_root(fk::RefPtr<Node> node) { m_root = node; }

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::mount(const char *path, fk::RefPtr<Node> node) {
  if (!path || !node)
    return fk::core::Error::InvalidParameter;
  if (strcmp(path, "/") == 0) {
    m_root = node;
    return {};
  }

  auto target_res = resolve_path(path);
  if (target_res.is_error())
    return target_res.error();

  m_mounts.push_back({path, node, target_res.value()});
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
VirtualFileSystem::resolve_path(const char *path, int depth) {
  if (depth > 8)
    return fk::core::Error::IOError;
  if (!path || !m_root)
    return fk::core::Error::InvalidParameter;

  fk::RefPtr<Node> current = m_root;
  const char *ptr = path;

  if (path[0] != '/') {
    auto *task = SchedulerManager::the().current();
    if (task && !task->cwd.empty()) {
      auto cwd_res = resolve_path(task->cwd.c_str(), depth + 1);
      if (cwd_res.is_ok())
        current = cwd_res.value();
    }
  } else {
    while (*ptr == '/')
      ptr++;
  }

  char resolved_path_str[512];
  fk::text::String current_node_path = current->get_path();

  if (current == m_root || current_node_path.is_empty()) {
    strcpy(resolved_path_str, "/");
  } else {
    strncpy(resolved_path_str, current_node_path.c_str(), 511);
    resolved_path_str[511] = '\0';
  }

  while (*ptr) {
    char name[256];
    size_t i = 0;
    while (*ptr && *ptr != '/' && i < 255)
      name[i++] = *ptr++;
    name[i] = '\0';

    while (*ptr == '/')
      ptr++;

    if (name[0] == '\0' || strcmp(name, ".") == 0)
      continue;

    if (strcmp(name, "..") == 0) {
      if (strcmp(resolved_path_str, "/") == 0)
        continue;
      char *last_slash = strrchr(resolved_path_str, '/', 512);
      if (last_slash) {
        if (last_slash == resolved_path_str)
          *(last_slash + 1) = '\0';
        else
          *last_slash = '\0';
      }
      auto restart_res = resolve_path(resolved_path_str, depth + 1);
      if (restart_res.is_error())
        return restart_res.error();
      current = restart_res.value();
      continue;
    }

    size_t cur_len = strlen(resolved_path_str);
    if (cur_len > 0 && resolved_path_str[cur_len - 1] != '/') {
      strcat(resolved_path_str, "/");
    }
    strcat(resolved_path_str, name);

    bool mounted = false;
    for (auto &m : m_mounts) {
      if (m.path == resolved_path_str) {
        current = m.source;
        mounted = true;
        break;
      }
    }

    if (!mounted) {
      auto lookup_res = current->lookup(name);
      if (lookup_res.is_error())
        return lookup_res.error();
      current = lookup_res.value();
    }

    // Symlink handling... (skipped for brevity but remains in file)

    int symlink_depth = 0;
    while (current->is_symlink() && symlink_depth < 8) {
      auto link_res = current->read_link();
      if (link_res.is_error())
        break;
      fk::text::String link = link_res.value();
      if (link.c_str()[0] == '/') {
        auto sub_res = resolve_path(link.c_str(), depth + 1);
        if (sub_res.is_error())
          return sub_res.error();
        current = sub_res.value();
        strncpy(resolved_path_str, link.c_str(), 511);
      } else {
        char parent_path[512];
        strncpy(parent_path, resolved_path_str, 511);
        char *last_slash = strrchr(parent_path, '/', 512);
        if (last_slash) {
          if (last_slash == parent_path)
            last_slash[1] = '\0';
          else
            *last_slash = '\0';
        }
        char full_link_path[512];
        snprintf(full_link_path, 512, "%s/%s", parent_path, link.c_str());
        auto sub_res = resolve_path(full_link_path, depth + 1);
        if (sub_res.is_error())
          return sub_res.error();
        current = sub_res.value();
        strncpy(resolved_path_str, full_link_path, 511);
      }
      symlink_depth++;
    }
  }
  return current;
}

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char *path, int flags) {
  auto node_result = resolve_path(path);
  if (node_result.is_error())
    return node_result.error();
  auto node = node_result.value();
  if ((flags & O_DIRECTORY) && !node->is_directory())
    return fk::core::Error::NotADirectory;
  return fk::make_ref<FileDescription>(node, flags).value();
}

fk::core::Result<void, fk::core::Error>

VirtualFileSystem::mkdir(const char *path, int mode) {

  if (!path)

    return fk::core::Error::InvalidParameter;

  const char *last_slash_ptr = strrchr(path, '/', strlen(path));

  if (!last_slash_ptr)
    return m_root->mkdir(path, mode).is_error()
               ? fk::core::Error::NotFound
               : fk::core::Result<void, fk::core::Error>({});
  size_t last_slash = last_slash_ptr - path;
  char parent_path[256];
  if (last_slash == 0)
    strcpy(parent_path, "/");
  else {
    size_t to_copy = last_slash < 255 ? last_slash : 255;
    strncpy(parent_path, path, to_copy);
    parent_path[to_copy] = '\0';
  }
  auto parent_node_res = resolve_path(parent_path);
  if (parent_node_res.is_error())
    return parent_node_res.error();
  auto mkdir_res = parent_node_res.value()->mkdir(last_slash_ptr + 1, mode);
  if (mkdir_res.is_error())
    return mkdir_res.error();
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::readdir(const char *path,
                           fk::containers::Vector<DirectoryEntry> &entries) {
  auto node_res = resolve_path(path);
  if (node_res.is_error())
    return node_res.error();
  (void)node_res.value()->list_dir(entries);
  return {};
}

fk::core::Result<size_t, fk::core::Error>
VirtualFileSystem::readdir(fk::RefPtr<FileDescription> description,
                           uint8_t *buffer, size_t max_bytes) {
  if (!buffer)
    return fk::core::Error::InvalidParameter;

  fk::containers::Vector<DirectoryEntry> entries;

  DirectoryEntry dot;
  strcpy(dot.name, ".");
  dot.type = 1;
  entries.push_back(dot);
  DirectoryEntry dotdot;
  strcpy(dotdot.name, "..");
  dotdot.type = 1;
  entries.push_back(dotdot);

  (void)description->node()->list_dir(entries);

  // Add mount points if we are at the right level
  fk::text::String current_path = description->node()->get_path();
  if (!current_path.is_empty()) {
    char path_buf[512];
    strncpy(path_buf, current_path.c_str(), 510);
    path_buf[510] = '\0';
    size_t path_len = strlen(path_buf);
    if (path_len > 0 && path_buf[path_len - 1] != '/') {
      path_buf[path_len] = '/';
      path_buf[path_len + 1] = '\0';
      path_len++;
    }

    for (auto &m : m_mounts) {
      if (m.path == "/" || m.path.length() <= path_len)
        continue;

      const char *m_path_str = m.path.c_str();
      if (strncmp(m_path_str, path_buf, path_len) == 0) {
        const char *rel = m_path_str + path_len;
        if (rel[0] != '\0' && !strchr(rel, '/', 256)) {
          bool exists = false;
          for (auto &e : entries) {
            if (strcmp(e.name, rel) == 0) {
              exists = true;
              break;
            }
          }
          if (!exists) {
            DirectoryEntry de;
            fk::memory::set(&de, 0, sizeof(de));
            strncpy(de.name, rel, sizeof(de.name) - 1);
            de.type = 1;
            entries.push_back(de);
          }
        }
      }
    }
  }

  uint64_t bytes_written = 0;
  uint64_t start_idx = description->offset();

  for (size_t i = start_idx; i < entries.size(); ++i) {
    auto &entry = entries[i];
    size_t name_len = strlen(entry.name);
    
    // Return "name\n" for each entry
    if (bytes_written + name_len + 1 > max_bytes)
      break;

    fk::memory::copy(buffer + bytes_written, entry.name, name_len);
    buffer[bytes_written + name_len] = '\n';
    bytes_written += name_len + 1;
    description->set_offset(i + 1);
  }

  return bytes_written;
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::stat(const char *path, struct stat *buf) {
  auto node_res = resolve_path(path);
  if (node_res.is_error())
    return node_res.error();
  auto node = node_res.value();
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
    if (buf->st_size == 0)
      buf->st_size = 4096;
  } else if (node->is_symlink()) {
    buf->st_mode = S_IFLNK | 0777;
    buf->st_nlink = 1;
  } else {
    buf->st_mode = S_IFREG | 0644;
    buf->st_nlink = 1;
  }
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::unmount(const char *path) {
  if (!path)
    return fk::core::Error::InvalidParameter;
  for (size_t i = 0; i < m_mounts.size(); ++i) {
    if (m_mounts[i].path == path) {
      for (size_t j = i + 1; j < m_mounts.size(); ++j)
        m_mounts[j - 1] = fk::types::move(m_mounts[j]);
      m_mounts.pop_back();
      return {};
    }
  }
  return fk::core::Error::NotFound;
}
