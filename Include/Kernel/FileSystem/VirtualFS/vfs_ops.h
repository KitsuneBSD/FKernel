#pragma once 

#include <LibC/stddef.h>
#include <Kernel/FileSystem/file_types.h>

struct VFSNode;
struct VFSFilesystem;

struct VFSOps {
    virtual ssize_t read(VFSNode* node, void* buf, size_t size, size_t offset) = 0;
    virtual ssize_t write(VFSNode* node, const void* buf, size_t size, size_t offset) = 0;
    virtual int mkdir(VFSNode* node, const char* name, FilePermissions perms) = 0;
    virtual int unlink(VFSNode* node, const char* name) = 0;
    virtual int rename(VFSNode* node, const char* old_name, const char* new_name) = 0;
    virtual int open(VFSNode* node, FileMode mode) = 0;
    virtual int close(VFSNode* node) = 0;
    virtual VFSNode* lookup(VFSNode* node, const char* name) = 0;

    virtual void init(VFSFilesystem* fs) { (void)fs; }
    virtual ~VFSOps() = default;
};