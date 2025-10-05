#pragma once 

#include <Kernel/Posix/errno.h>
#include <Kernel/FileSystem/VirtualFS/vfs_node.h>
#include <Kernel/FileSystem/VirtualFS/vfs_ops.h>
#include <LibFK/Algorithms/log.h>

extern VFSNode* g_devfs_root;

struct DevFSOps final : public VFSOps {
    ssize_t read(VFSNode* node, void* buf, size_t size, size_t offset) override;
    ssize_t write(VFSNode* node, const void* buf, size_t size, size_t offset) override;
    int mkdir(VFSNode*, const char*, FilePermissions) override { return -ENOTDIR; }
    int unlink(VFSNode*, const char*) override { return -EPERM; }
    int rename(VFSNode*, const char*, const char*) override { return -EPERM; }
    int open(VFSNode* node, FileMode) override { (void)node; return 0; }
    int close(VFSNode* node) override { (void)node; return 0; }
    VFSNode* lookup(VFSNode* node, const char* name) override;
};