#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/retain_ptr.h>

#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/pair.h>

#include <Kernel/Posix/sys/stat.h>

class VirtualFileSystem {
public:
  struct Mount {
    fk::text::String path;
    fk::RefPtr<Node> source;
    fk::RefPtr<Node> target;
  };

private:
  fk::RefPtr<Node> m_root;
  fk::containers::Vector<Mount> m_mounts;
  VirtualFileSystem() = default;

public:
  static void initialize();
  static VirtualFileSystem &the();

  void mount_root(fk::RefPtr<Node> node);
  fk::core::Result<void, fk::core::Error>
  mount(const char *path, fk::RefPtr<Node> node);

  fk::core::Result<fk::RefPtr<FileDescription>, fk::core::Error>
  open(const char *path, int flags);

  fk::core::Result<void, fk::core::Error>
  mkdir(const char *path, int mode);

  fk::core::Result<void, fk::core::Error>
  readdir(const char *path, fk::containers::Vector<DirectoryEntry>& entries);

  fk::core::Result<size_t, fk::core::Error>
  readdir(fk::RefPtr<FileDescription> description, uint8_t* buffer, size_t max_bytes);

  fk::core::Result<void, fk::core::Error>
  stat(const char *path, struct stat *buf);

  fk::core::Result<void, fk::core::Error> unmount(const char* path);

  fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  resolve_path(const char *path, int depth = 0);

private:
};
