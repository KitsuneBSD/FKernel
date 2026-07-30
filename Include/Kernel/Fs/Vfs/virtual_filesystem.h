#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Vfs/path_resolver.h>
#include <LibFK/Core/result.h>

#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/pair.h>

#include <Kernel/Posix/sys/stat.h>

#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {

class Dentry;

class VirtualFileSystem {
private:
  fk::synchronization::Spinlock m_lock;
  PathResolver m_resolver;
  fk::RefPtr<Dentry> m_root;
  VirtualFileSystem();

public:
  static void initialize();
  static VirtualFileSystem& the();

  fk::RefPtr<Dentry> root() const { return m_root; }

  void mount_root(fk::RefPtr<Node> node);
  fk::core::Result<void, fk::core::Error> mount(const char* path, fk::RefPtr<Node> node,
                                                 const char* fstype = "auto");

  // Iterate all recorded mount points (path, fstype). Callback receives c-strings.
  static void for_each_mount(void (*cb)(const char* path, const char* fstype, void* ctx), void* ctx);

  // Clone the mount namespace for the current task (or global if no ns set).
  fk::RefPtr<MountNamespace> clone_mount_namespace();

  // Return the device ID for the mount point whose path is the longest prefix of `path`.
  static uint32_t dev_id_for_path(const char* path);

  fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error> open(const char* path, int flags);

  fk::core::Result<void, fk::core::Error> mkdir(const char* path, int mode);

  fk::core::Result<void, fk::core::Error> mkfifo(const char* path, int mode);

  fk::core::Result<void, fk::core::Error> symlink(const char* path, const char* target);

  fk::core::Result<void, fk::core::Error> rmdir(const char* path);

  fk::core::Result<void, fk::core::Error> unlink(const char* path);

  fk::core::Result<void, fk::core::Error> link(const char* path, const char* target);

  fk::core::Result<void, fk::core::Error> rename(const char* old_path, const char* new_path);

  fk::core::Result<void, fk::core::Error> readdir(const char* path,
                                                  fk::containers::Vector<DirectoryEntry>& entries);

  fk::core::Result<size_t, fk::core::Error> readdir(fk::RefPtr<FileDescription> description,
                                                    uint8_t* buffer, size_t max_bytes);

  fk::core::Result<void, fk::core::Error> stat(const char* path, struct stat* buf);

  fk::core::Result<void, fk::core::Error> truncate(const char* path, uint64_t size);
  fk::core::Result<void, fk::core::Error> fsync(fk::RefPtr<FileDescription> desc);
  fk::core::Result<void, fk::core::Error> chmod(const char* path, uint32_t mode);
  fk::core::Result<void, fk::core::Error> chown(const char* path, uint32_t uid, uint32_t gid);

  fk::core::Result<void, fk::core::Error> unmount(const char* path);

  fk::core::Result<void, fk::core::Error> pivot_root(const char* new_root, const char* put_old);

  fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
  resolve_path(const char* path, fk::RefPtr<Dentry> base = nullptr, int depth = 0);

  fk::core::Result<fk::RefPtr<Dentry>, fk::core::Error>
  resolve_path_unlocked(const char* path, fk::RefPtr<Dentry> base = nullptr, int depth = 0);

private:
  fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
  resolve_path_to_parent(const char* path, int depth = 0);

  fk::core::Result<fk::utilities::Pair<fk::RefPtr<Dentry>, fk::text::String>, fk::core::Error>
  resolve_path_to_parent_unlocked(const char* path, int depth = 0);

  void add_directory_entry(fk::containers::Vector<DirectoryEntry>& entries,
                           const DirectoryEntry& entry);
};

} // namespace fkernel
