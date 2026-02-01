#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
VirtualFileSystem::resolve_path(const char *path, fk::RefPtr<Dentry> base, int depth) {
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    return resolve_path_unlocked(path, base, depth);
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
VirtualFileSystem::resolve_path_unlocked(const char *path, fk::RefPtr<Dentry> base, int depth) {
  if (depth > 8) return fk::core::Error::IOError;
  if (!path || !m_root) return fk::core::Error::InvalidParameter;

  fk::RefPtr<Dentry> current = m_root;
  const char *ptr = path;

  if (path[0] == '/') {
      while (*ptr == '/') ptr++;
  } else if (base) {
      current = base;
  } else {
      auto *task = SchedulerManager::the().current();
      if (task && !task->resources.files.cwd.empty()) {
          auto cwd_res = resolve_path_unlocked(task->resources.files.cwd.c_str(), nullptr, depth + 1);
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

    int symlink_depth = 0;
    while (current->top_node() && current->top_node()->is_symlink() && symlink_depth < 8) {
        auto link_res = current->top_node()->read_link();
        if (link_res.is_error()) return link_res.error();
        
        fk::text::String link = link_res.value();
        if (link.c_str()[0] == '/') {
            auto sub_res = resolve_path_unlocked(link.c_str(), nullptr, depth + 1);
            if (sub_res.is_error()) return sub_res.error();
            current = sub_res.value();
        } else {
            auto sub_res = resolve_path_unlocked(link.c_str(), current->parent(), depth + 1);
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
    fk::synchronization::ScopedLockIRQ lock(m_lock);
    return resolve_path_to_parent_unlocked(path, depth);
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
VirtualFileSystem::resolve_path_to_parent_unlocked(const char *path, int depth) {
    if (!path || path[0] == '\0') return fk::core::Error::InvalidParameter;

    char parent_path[512];
    strncpy(parent_path, path, 511);
    parent_path[511] = '\0';

    size_t len = strlen(parent_path);
    while (len > 1 && parent_path[len - 1] == '/') {
        parent_path[len - 1] = '\0';
        len--;
    }

    char *last_slash = strrchr(parent_path, '/', 512);
    fk::text::String name;

    if (!last_slash) {
        name = parent_path;
        auto *task = SchedulerManager::the().current();
        auto cwd_res = resolve_path_unlocked(task ? task->resources.files.cwd.c_str() : "/", nullptr, depth + 1);
        if (cwd_res.is_error()) return cwd_res.error();
        return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(cwd_res.value(), name);
    }

    if (last_slash == parent_path) {
        name = last_slash + 1;
        return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(m_root, name);
    }

    *last_slash = '\0';
    name = last_slash + 1;
    auto parent_res = resolve_path_unlocked(parent_path, nullptr, depth + 1);
    if (parent_res.is_error()) return parent_res.error();
    return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(parent_res.value(), name);
}

} // namespace fkernel
