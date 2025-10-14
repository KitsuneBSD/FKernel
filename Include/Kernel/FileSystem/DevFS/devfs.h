#pragma once

#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/DevFS/device.h>

struct DevFS
{
private:
    RetainPtr<VNode> d_root;
    static_vector<Device, 256> d_devices;

public:
    DevFS();
    static DevFS &the();

    RetainPtr<VNode> root();

    int register_device_static(const char *name, VNodeType type, VNodeOps *ops, void *driver_data);
    int register_device(const char *name, VNodeType type, VNodeOps *ops, void *driver_data, bool has_multiple = false);
    int unregister_device(const char *name);

    static int devfs_lookup(VNode *vnode, const char *name, RetainPtr<VNode> &out);
    static int devfs_readdir(VNode *vnode, void *buffer, size_t max_entries);
    static int devfs_open(VNode *vnode, int flags);
    static int devfs_close(VNode *vnode);
    static int devfs_read(VNode *vnode, void *buffer, size_t size, size_t offset);
    static int devfs_write(VNode *vnode, const void *buffer, size_t size, size_t offset);
    static int devfs_create(VNode *dir, const char *name, VNodeType type, RetainPtr<VNode> &out);
    static int devfs_unlink(VNode *dir, const char *name);

    static VNodeOps ops;
};