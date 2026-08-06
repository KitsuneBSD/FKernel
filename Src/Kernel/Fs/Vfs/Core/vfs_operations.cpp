#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Virtual/PipeFs/pipe_node.h>
#include <Kernel/Posix/sys/time.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

static constexpr size_t BLOCK_SECTOR_SIZE = 512;

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
  auto open_result = node->on_open();
  if (open_result.is_error())
    return open_result.error();
  return fk::make_ref<FileDescription>(dentry, flags).value();
}

fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
VirtualFileSystem::open(const char* path, int flags) {
  fk::algorithms::kdebug("VFS", "open(%s, flags=%d)", path, flags);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);

  if (dentry_res.is_ok()) {
    if (flags & O_EXCL)
      return fk::core::Error::AlreadyExists;
    fk::algorithms::ktrace("VFS", "open(%s): found existing dentry flags=0x%x", path, flags);
    return open_existing(dentry_res.value(), flags);
  }

  if (!(flags & O_CREAT)) {
    fk::algorithms::kwarn("VFS", "open: resolve failed for %s", path);
    return dentry_res.error();
  }

  fk::algorithms::ktrace("VFS", "open(%s): O_CREAT — creating new file", path);
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
  fk::algorithms::kdebug("VFS", "mkdir(%s)", path);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error()) {
    fk::algorithms::kwarn("VFS", "mkdir: resolve failed for %s", path);
    return parent_res.error();
  }

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto node = find_writable_directory_node(parent_dentry);
  if (!node) {
    return fk::core::Error::NotFound;
  }

  auto new_node = TRY(node->mkdir(name.c_str(), mode));
  auto child_dentry = TRY(Dentry::create(name, parent_dentry));
  child_dentry->push_node(new_node);
  parent_dentry->add_child(child_dentry);
  return {};
}

fk::core::Result<void, fk::core::Error> VirtualFileSystem::mkfifo(const char* path, [[maybe_unused]] int mode) {
  fk::algorithms::kdebug("VFS", "mkfifo(%s)", path);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto parent_res = resolve_path_to_parent_unlocked(path);
  if (parent_res.is_error()) {
    fk::algorithms::kwarn("VFS", "mkfifo: resolve failed for %s", path);
    return parent_res.error();
  }

  auto parent_dentry = parent_res.value().first;
  auto name = parent_res.value().second;

  auto pipe_res = PipeNode::create();
  if (pipe_res.is_error())
    return pipe_res.error();
  auto pipe = pipe_res.value();

  auto child_dentry = TRY(Dentry::create(name, parent_dentry));
  child_dentry->push_node(pipe);
  parent_dentry->add_child(child_dentry);
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
  fk::algorithms::kdebug("VFS", "unlink(%s)", path);
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
  fk::algorithms::kdebug("VFS", "rename(%s, %s)", old_path, new_path);
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
  fk::algorithms::kdebug("VFS", "stat(%s)", path);
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();

  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;

  fk::memory::set(buf, 0, sizeof(struct stat));
  buf->st_dev = VirtualFileSystem::dev_id_for_path(path);
  buf->st_ino = node->inode();
  buf->st_size = node->size();
  buf->st_blksize = PAGE_SIZE;
  buf->st_blocks = (buf->st_size + BLOCK_SECTOR_SIZE - 1) / BLOCK_SECTOR_SIZE;
  buf->st_uid = node->node_uid();
  buf->st_gid = node->node_gid();
  buf->st_mode = node->node_mode();
  buf->st_atime = (node->atime().tv_sec != 0 || node->atime().tv_nsec != 0)
                      ? node->atime().tv_sec
                      : static_cast<time_t>(TickManager::the().get_ticks());
  buf->st_mtime = (node->mtime().tv_sec != 0 || node->mtime().tv_nsec != 0)
                      ? node->mtime().tv_sec
                      : static_cast<time_t>(TickManager::the().get_ticks());
  buf->st_ctime = (node->ctime().tv_sec != 0 || node->ctime().tv_nsec != 0)
                      ? node->ctime().tv_sec
                      : static_cast<time_t>(TickManager::the().get_ticks());

  if (node->is_directory()) {
    buf->st_nlink = 2;
    if (buf->st_size == 0)
      buf->st_size = PAGE_SIZE;
  } else {
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

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::chmod(const char* path, uint32_t mode) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();
  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;
  node->set_permissions(mode);
  return {};
}

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::chown(const char* path, uint32_t uid, uint32_t gid) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error())
    return dentry_res.error();
  auto node = dentry_res.value()->top_node();
  if (!node)
    return fk::core::Error::NotFound;
  node->set_owner(uid, gid);
  return {};
}

} // namespace fkernel
