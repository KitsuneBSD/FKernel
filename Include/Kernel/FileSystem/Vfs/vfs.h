#pragma once 

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Traits/type_traits.h>

#include <Kernel/FileSystem/file_types.h>
#include <Kernel/FileSystem/Vfs/vfs_fs.h>
#include <Kernel/FileSystem/Vfs/vfs_ops.h>
#include <Kernel/FileSystem/Vfs/vfs_node.h>

struct VFS {
    static void init();
    static VFSNode* root();

    static int mount(VFSNode* node, VFSNode* mountpoint);
    static int mount(VFSNode* mountpoint, VFSFilesystem* fs);
    static int mount(VFSNode* fs_root, const char* mount_path);

    static VFSNode* resolve_path(const char* path);

    static int mkdir(const char* path, FilePermissions perms);
    static int unlink(const char* path);
    static int rename(const char* old_path, const char* new_path);

    static ssize_t read(VFSNode* node, void* buf, size_t size, size_t offset);
    static ssize_t write(VFSNode* node, const void* buf, size_t size, size_t offset);

    static int open(VFSNode* node, FileMode mode);
    static int close(VFSNode* node);
};