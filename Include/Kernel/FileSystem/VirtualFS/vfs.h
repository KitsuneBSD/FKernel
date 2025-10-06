#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Traits/type_traits.h>

#include <Kernel/FileSystem/file_types.h>
#include <Kernel/FileSystem/VirtualFS/vfs_fs.h>
#include <Kernel/FileSystem/VirtualFS/vfs_ops.h>
#include <Kernel/FileSystem/VirtualFS/vfs_node.h>

class VFS
{

private:
    VFS() { init(); }

    VFSNode *create_node(const char *name, FileType type, FilePermissions perms);
    VFSNode *lookup_child(VFSNode *parent, const char *name);

    bool m_is_initialized = false;
    VFSNode *m_root{nullptr};

    // Proibindo cópia/movimento
    VFS(const VFS &) = delete;
    VFS &operator=(const VFS &) = delete;
    VFS(VFS &&) = delete;
    VFS &operator=(VFS &&) = delete;

public:
    static VFS &the()
    {
        static VFS instance; // Singleton RAII
        return instance;
    }

    void init();

    VFSNode *root() const { return m_root; }

    int mkdir(const char *path, FilePermissions perms);
    int unlink(const char *path);
    int rename(const char *old_path, const char *new_path);

    ssize_t read(VFSNode *node, void *buf, size_t size, size_t offset);
    ssize_t write(VFSNode *node, const void *buf, size_t size, size_t offset);

    int open(VFSNode *node, FileMode mode);
    int close(VFSNode *node);

    int mount(VFSNode *node, VFSNode *mountpoint);
    int mount(VFSNode *mountpoint, VFSFilesystem *fs);
    int mount(VFSNode *fs_root, const char *mount_path);

    VFSNode *resolve_path(const char *path);
};