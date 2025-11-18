#pragma once

#include <Kernel/FileSystem/DevFS/device.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

struct DevFS {
private:
  fk::memory::RetainPtr<VNode> d_root;
  fk::containers::static_vector<Device, 256> d_devices;

public:
  DevFS();
  static DevFS &the();

  template <typename Func> void for_each_device(Func f) {
    for (auto &dev : d_devices) {
      f(dev);
    }
  }

  fk::memory::RetainPtr<VNode> root();

  int register_device_static(const char *name, VNodeType type, VNodeOps *ops,
                             void *driver_data);
  int register_device(const char *name, VNodeType type, VNodeOps *ops,
                      void *driver_data, bool has_multiple = false,
                      bool start_with_zero = true);
  int unregister_device(const char *name);

  static int devfs_lookup(VNode *vnode, FileDescriptor *fd, const char *name,
                          fk::memory::RetainPtr<VNode> &out);
  static int devfs_readdir(VNode *vnode, FileDescriptor *fd, void *buffer,
                           size_t max_entries);
  static int devfs_open(VNode *vnode, FileDescriptor *fd, int flags);
  static int devfs_close(VNode *vnode, FileDescriptor *fd);
  static int devfs_read(VNode *vnode, FileDescriptor *fd, void *buffer,
                        size_t size, size_t offset);
  static int devfs_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                         size_t size, size_t offset);
  static int devfs_create(VNode *dir, FileDescriptor *fd, const char *name,
                          VNodeType type, fk::memory::RetainPtr<VNode> &out);
  static int devfs_unlink(VNode *dir, FileDescriptor *fd, const char *name);

  static VNodeOps ops;
};
