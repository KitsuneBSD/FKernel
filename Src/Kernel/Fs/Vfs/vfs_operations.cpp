#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibC/string.h>
#include <LibFK/Utilities/Memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char* path, int flags) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();

  auto dentry = dentry_res.value();
  auto node = dentry->top_node();

  if (!node)
    return fk::core::Error::NotFound;
  if ((flags & O_DIRECTORY) && !node->is_directory())
    return fk::core::Error::NotADirectory;

  return fk::make_ref<FileDescription>(dentry, flags).value();
}

static fk::RefPtr<Node> find_writable_directory_node(fk::RefPtr<Dentry> dentry) {
  auto& nodes = dentry->nodes();
  for (size_t i = nodes.size(); i > 0; --i) {
    auto node = nodes[i - 1];
    if (node->is_directory()) {
      return node;
    }
  }
  return nullptr;
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::mkdir(const char* path, int mode) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  fk::algorithms::kdebug("VFS", "mkdir: path=%s, parent_dentry=%s, name=%s", path,
                         parent_dentry->get_path().c_str(), name.c_str());

  auto node = find_writable_directory_node(parent_dentry);
  if (!node) {
    fk::algorithms::kdebug("VFS", "mkdir: parent dentry has no writable directory node for %s",
                           path);
    return fk::core::Error::NotFound;
  }

  fk::algorithms::kdebug("VFS", "mkdir: using node=%s, is_dir=%d", node->name().c_str(),
                         node->is_directory());

  TRY(node->mkdir(name.c_str(), mode));
  return {};
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::symlink(const char* path,
                                                                   const char* target) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  return node->symlink(name.c_str(), target);
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::rmdir(const char* path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  return node->rmdir(name.c_str());
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::unlink(const char* path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  return node->unlink(name.c_str());
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::link(const char* path,
                                                                const char* target) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  return node->link(name.c_str(), target);
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::rename(const char* old_path,
                                                                  const char* new_path) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  auto old_parent_res = resolve_path_to_parent_unlocked(old_path);
  if (old_parent_res.is_error())
    return old_parent_res.error();

  auto old_parent_dentry = old_parent_res.value().first;
  auto old_name = old_parent_res.value().second;

  auto new_parent_res = resolve_path_to_parent_unlocked(new_path);
  if (new_parent_res.is_error())
    return new_parent_res.error();

  auto new_parent_dentry = new_parent_res.value().first;
  auto new_name = new_parent_res.value().second;

  auto old_node = old_parent_dentry->top_node();
  if (!old_node)
    return fk::core::Error::NotFound;

  return old_node->rename(old_name.c_str(), new_name.c_str());
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::stat(const char* path,
                                                                struct stat* buf) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();

  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;

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

} // namespace fkernel
