#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <LibC/string.h>
#include <LibFK/Utilities/Memory.h>

namespace fkernel {

static fk::RefPtr<Node> find_writable_directory_node(fk::RefPtr<Dentry> dentry);

static fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
open_existing(fk::RefPtr<fkernel::Dentry> dentry, int flags) {
  auto node = dentry->top_node();
  if (!node)
    return fk::core::Error::NotFound;
  if ((flags & O_DIRECTORY) && !node->is_directory())
    return fk::core::Error::NotADirectory;
  if ((flags & O_TRUNC) && !node->is_directory())
    node->truncate(0);
  return fk::make_ref<FileDescription>(dentry, flags).value();
}

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char* path, int flags) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);

  if (dentry_res.is_ok()) {
    if (flags & O_EXCL)
      return fk::core::Error::AlreadyExists;
    return open_existing(dentry_res.value(), flags);
  }

  if (!(flags & O_CREAT))
    return dentry_res.error();

  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error())
    return parent_res.error();

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;
  auto parent_node = find_writable_directory_node(parent_dentry);
  if (!parent_node)
    return fk::core::Error::NotFound;

  auto new_node = TRY(parent_node->create_child(name.c_str(), 0644));
  auto child_dentry = TRY(Dentry::create(name, parent_dentry));
  child_dentry->push_node(new_node);
  parent_dentry->add_child(child_dentry);

  return fk::make_ref<FileDescription>(child_dentry, flags).value();
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

  auto node = find_writable_directory_node(parent_dentry);
  if (!node) {
    return fk::core::Error::NotFound;
  }

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

  auto node = find_writable_directory_node(parent_dentry);
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
  buf->st_ino = node->inode();
  buf->st_atime = buf->st_mtime = buf->st_ctime = TickManager::the().get_ticks();

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

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::truncate(const char* path, uint64_t size) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();
  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;
  return node->truncate(size);
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::fsync(fk::RefPtr<FileDescription> desc) {
  if (!desc)
    return fk::core::Error::InvalidHandle;
  auto node = desc->node();
  if (!node)
    return fk::core::Error::InvalidHandle;
  return node->fsync();
}

} // namespace fkernel
