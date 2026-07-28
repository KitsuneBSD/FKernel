#include <Kernel/Fs/Vfs/path_resolver.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

PathResolver::PathResolver(VirtualFileSystem& vfs, fk::synchronization::Spinlock& lock)
    : m_vfs(vfs), m_lock(lock) {}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
PathResolver::resolve(const char* path, fk::RefPtr<Dentry> base, int depth) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return resolve_unlocked(path, base, depth);
}

fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
PathResolver::resolve_unlocked(const char* path, fk::RefPtr<Dentry> base, int depth) {
  if (depth > 8) return fk::core::Error::IOError;

  if (!path) return fk::core::Error::InvalidParameter;

  auto* task = SchedulerManager::the().current();
  fk::RefPtr<Dentry> root = m_vfs.root();
  if (task && task->resources.files.root)
    root = task->resources.files.root;
  if (!root) return fk::core::Error::InvalidParameter;

  fk::RefPtr<Dentry> current = root;
  const char* ptr = path;

  if (path[0] == '/') {
    while (*ptr == '/') ++ptr;
  } else if (base) {
    current = base;
  } else {
    if (task && !task->resources.files.cwd.empty()) {
      auto cwd_res = resolve_unlocked(task->resources.files.cwd.c_str(), nullptr, depth + 1);
      if (cwd_res.is_ok()) current = cwd_res.value();
    }
  }

  while (*ptr) {
    char name[256];
    size_t i = 0;
    while (*ptr && *ptr != '/' && i < sizeof(name) - 1)
      name[i++] = *ptr++;
    name[i] = '\0';

    while (*ptr == '/') ++ptr;

    if (name[0] == '\0' || fk::memory::compare(name, ".") == 0)
      continue;

    if (fk::memory::compare(name, "..") == 0) {
      if (current.get() != root.get() && current->parent()) current = current->parent();
      continue;
    }

    auto next_res = current->lookup(name);
    if (next_res.is_error()) return next_res.error();
    current = next_res.value();

    for (int sl = 0; current->top_node() && current->top_node()->is_symlink() && sl < 8; ++sl) {
      auto link_res = current->top_node()->read_link();
      if (link_res.is_error()) return link_res.error();
      auto& link = link_res.value();
      auto symlink_base = link.c_str()[0] == '/' ? nullptr : current->parent();
      auto resolved = resolve_unlocked(link.c_str(), symlink_base, depth + 1);
      if (resolved.is_error()) return resolved.error();
      current = resolved.value();
    }
  }

  return current;
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
PathResolver::resolve_to_parent(const char* path, int depth) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  return resolve_to_parent_unlocked(path, depth);
}

fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
PathResolver::resolve_to_parent_unlocked(const char* path, int depth) {
  if (!path || path[0] == '\0') return fk::core::Error::InvalidParameter;

  char parent_path[512];
  fk::memory::copy_n(parent_path, path, 511);
  parent_path[511] = '\0';

  size_t len = fk::memory::length(parent_path);
  while (len > 1 && parent_path[len - 1] == '/') {
    parent_path[len - 1] = '\0';
    len--;
  }

  char* last_slash = strrnchr(parent_path, '/', 512);
  fk::text::String name;

  if (!last_slash) {
    name = parent_path;
    auto* task = SchedulerManager::the().current();
    auto cwd_res = resolve_unlocked(task ? task->resources.files.cwd.c_str() : "/", nullptr, depth + 1);
    if (cwd_res.is_error()) return cwd_res.error();
    return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(cwd_res.value(), name);
  }

  if (last_slash == parent_path) {
    name = last_slash + 1;
    return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(m_vfs.root(), name);
  }

  *last_slash = '\0';
  name = last_slash + 1;
  auto parent_res = resolve_unlocked(parent_path, nullptr, depth + 1);
  if (parent_res.is_error()) return parent_res.error();
  return fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>(parent_res.value(), name);
}

} // namespace fkernel
