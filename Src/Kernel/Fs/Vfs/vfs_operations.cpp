#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <LibC/string.h>
#include <LibFK/Utilities/Memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char *path, int flags) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error()) return dentry_res.error();
  
  auto dentry = dentry_res.value();
  auto node = dentry->top_node();
  
  if (!node) return fk::core::Error::NotFound;
  if ((flags & O_DIRECTORY) && !node->is_directory()) return fk::core::Error::NotADirectory;
  
  return fk::make_ref<FileDescription>(dentry, flags).value();
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::mkdir(const char *path, int mode) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
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
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error()) return parent_res.error();
  
  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = parent_dentry->top_node();
  if (!node) return fk::core::Error::NotFound;

  return node->symlink(name.c_str(), target);
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::stat(const char *path, struct stat *buf) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
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

} // namespace fkernel
