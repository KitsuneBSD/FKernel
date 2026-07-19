#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>

namespace fkernel {

static fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
resolve_symlink(VirtualFileSystem& vfs, fk::RefPtr<Dentry> dentry, int depth) {
  auto link_res = dentry->top_node()->read_link();
  if (link_res.is_error())
    return link_res.error();
  auto& link = link_res.value();
  auto base = link.c_str()[0] == '/' ? nullptr : dentry->parent();
  return vfs.resolve_path_unlocked(link.c_str(), base, depth + 1);
}

static const char* skip_slashes(const char* ptr) {
  while (*ptr == '/') ++ptr;
  return ptr;
}

static const char* read_component(const char* ptr, char* out, size_t max) {
  size_t i = 0;
  while (*ptr && *ptr != '/' && i < max - 1)
    out[i++] = *ptr++;
  out[i] = '\0';
  return ptr;
}

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
    ptr = skip_slashes(ptr);
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
    ptr = read_component(ptr, name, sizeof(name));

    if (name[0] == '\0' || strcmp(name, ".") == 0) {
      ptr = skip_slashes(ptr);
      continue;
    }

    if (strcmp(name, "..") == 0) {
      if (current->parent()) current = current->parent();
      ptr = skip_slashes(ptr);
      continue;
    }

    auto next_res = current->lookup(name);
    if (next_res.is_error()) return next_res.error();
    current = next_res.value();

    for (int sl = 0; current->top_node() && current->top_node()->is_symlink() && sl < 8; ++sl) {
      auto res = resolve_symlink(*this, current, depth);
      if (res.is_error()) return res.error();
      current = res.value();
    }

    ptr = skip_slashes(ptr);
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
