#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_flags.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>
#include <Kernel/FileSystem/VirtualFS/filesystem.h> // Required for fkernel::fs::Filesystem
#include <LibFK/Memory/own_ptr.h> // Required for fk::memory::OwnPtr

struct Mountpoint {
  fk::text::fixed_string<256> m_name;
  fk::memory::RetainPtr<VNode> m_root;
  fk::memory::OwnPtr<fkernel::fs::Filesystem> m_fs_instance; // Owns the filesystem instance

  Mountpoint() = default;
  Mountpoint(const char *name, fk::memory::RetainPtr<VNode> r, fk::memory::OwnPtr<fkernel::fs::Filesystem> fs_instance)
      : m_name(name), m_root(r), m_fs_instance(fk::types::move(fs_instance)) {}

  // Move constructor
  Mountpoint(Mountpoint&& other) noexcept
      : m_name(fk::types::move(other.m_name))
      , m_root(fk::types::move(other.m_root))
      , m_fs_instance(fk::types::move(other.m_fs_instance)) {}

  // Move assignment operator
  Mountpoint& operator=(Mountpoint&& other) noexcept {
      if (this != &other) {
          m_name = fk::types::move(other.m_name);
          m_root = fk::types::move(other.m_root);
          m_fs_instance = fk::types::move(other.m_fs_instance);
      }
      return *this;
  }

  // Delete copy constructor and copy assignment operator
  Mountpoint(const Mountpoint&) = delete;
  Mountpoint& operator=(const Mountpoint&) = delete;
};

class VirtualFS {
private:
  VirtualFS() = default;
  fk::memory::RetainPtr<VNode> v_root;
  fk::memory::RetainPtr<VNode> v_cwd;
  // TODO: Apply b+ tree instead static_vector
  fk::containers::static_vector<Mountpoint, 8> v_mounts;

  fk::memory::RetainPtr<VNode> resolve_path(const char *path,
                                            fk::memory::RetainPtr<VNode> cwd);
  void set_cwd(fk::memory::RetainPtr<VNode> vnode);

public:
  static VirtualFS &the() {
    static VirtualFS inst;
    return inst;
  }
  int mount(const char *name, fk::memory::RetainPtr<VNode> root, fk::memory::OwnPtr<fkernel::fs::Filesystem> fs_instance = nullptr);
  int lookup(const char *path, fk::memory::RetainPtr<VNode> &out);
  int open(const char *path, int flags);
  int read(int file_descriptor, void *buf, size_t sz, size_t off);
  int write(int file_descriptor, const void *buf, size_t sz, size_t off);

  fk::memory::RetainPtr<VNode> root() const;
  fk::memory::RetainPtr<VNode> cwd() const;
};
