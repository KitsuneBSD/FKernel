#pragma once

#pragma once
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/retain_ptr.h>

struct RamFS {
private:
  fk::memory::RetainPtr<VNode> r_root;

public:
  RamFS();
  static RamFS &the();
  fk::memory::RetainPtr<VNode> root();

  // Funções de operação para VNodeOps
  static int ramfs_read(VNode *vnode, FileDescriptor *fd, void *buffer,
                        size_t size, size_t offset);
  static int ramfs_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                         size_t size, size_t offset);
  static int ramfs_open(VNode *vnode, FileDescriptor *fd, int flags);
  static int ramfs_close(VNode *vnode, FileDescriptor *fd);
  static int ramfs_lookup(VNode *vnode, FileDescriptor *fd, const char *name,
                          fk::memory::RetainPtr<VNode> &out);
  static int ramfs_create(VNode *vnode, FileDescriptor *fd, const char *name,
                          VNodeType type, fk::memory::RetainPtr<VNode> &out);
  static int ramfs_readdir(VNode *vnode, FileDescriptor *fd, void *buffer,
                           size_t max_entries);
  static int ramfs_unlink(VNode *vnode, FileDescriptor *fd, const char *name);
  static VNodeOps ops;
};
