#pragma once 

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/DevFS/devfs_ops.h>
#include <Kernel/Posix/errno.h>

struct NullDeviceOps : public VFSOps {
    ssize_t read(VFSNode* node, void* buf, size_t size, size_t offset) override {
        (void)node; (void)buf; (void)size; (void)offset;
        return 0; // EOF sempre
    }

    ssize_t write(VFSNode* node, const void* buf, size_t size, size_t offset) override {
        (void)node; (void)buf; (void)size; (void)offset;
        return size;
    }

    int mkdir(VFSNode*, const char*, FilePermissions) override { return -ENOTDIR; }
    int unlink(VFSNode*, const char*) override { return -EPERM; }
    int rename(VFSNode*, const char*, const char*) override { return -EPERM; }
    int open(VFSNode* node, FileMode) override { (void)node; return 0; }
    int close(VFSNode* node) override { (void)node; return 0; }

    VFSNode* lookup(VFSNode* node, const char* name) override {
        (void)node; (void)name;
        return nullptr;
    }
};

static NullDeviceOps null_ops;
