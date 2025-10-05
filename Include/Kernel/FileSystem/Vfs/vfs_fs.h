#pragma once 

#include <Kernel/FileSystem/Vfs/vfs_ops.h>
#include <Kernel/FileSystem/Vfs/vfs_node.h>
#include <LibFK/Container/fixed_string.h>

constexpr size_t VFS_MAX_MOUNTS = (2 << 16);

struct VFSFilesystem {
    fixed_string<256> name;
    VFSNode *root;
    VFSOps *ops{nullptr};
    void* fs_private{nullptr};

    VFSFilesystem(const char* n, VFSNode* r, VFSOps* o, void* priv = nullptr)
        : name(n), root(r), ops(o), fs_private(priv) {}
};

struct VFSMount {
    VFSNode* mountpoint;
    VFSFilesystem* fs;
};