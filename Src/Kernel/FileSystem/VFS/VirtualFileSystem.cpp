#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <Kernel/FileSystem/VFS/VirtualFileSystem.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>
#include <LibFK/new.h>

namespace FileSystem {

VFS::VFS()
    : mount_points_()
{
}

int VFS::mount(char const* path, VNode* root)
{
    if (FK::alert_if(path == nullptr, "VFS: Mount path is null"))
        return -1;

    if (FK::alert_if(root == nullptr, "VFS: Root VNode is null"))
        return -1;

    void* mem = Falloc(sizeof(MountPoint));

    if (FK::alert_if(mem == nullptr, "VFS: Failed to allocate mount point"))
        return -1;

    auto* mp = new (mem) MountPoint();

    LibC::strncpy(mp->path, path, MAX_PATH_LEN);
    mp->root_vnode = root;

    mount_points_.append(mp);
    return 0;
}

MountPoint* VFS::find_mount_point(char const* path)
{
    if (FK::alert_if(path == nullptr, "VFS: path is null"))
        return nullptr;

    MountPoint* best_match = nullptr;

    LibC::size_t best_len = 0;

    for (auto& mp : mount_points_) {
        LibC::size_t len = LibC::strlen(mp.path);

        if (len > best_len && LibC::strncmp(path, mp.path, len) == 0) {
            if (path[len] == '/' || path[len] == '\0') {
                best_match = &mp;
                best_len = len;
            }
        }
    }

    return best_match;
}

VNode* VFS::resolve_path(char const* path, char* relative_path_out, LibC::size_t relative_path_out_size)
{
    if (FK::alert_if(path == nullptr, "VFS: resolve_path - path is null"))
        return nullptr;

    if (FK::alert_if(relative_path_out == nullptr, "VFS: resolve_path - out buffer is null"))
        return nullptr;

    MountPoint* mp = find_mount_point(path);
    if (FK::alert_if(mp == nullptr, "VFS: resolve_path - mount point not found"))
        return nullptr;

    LibC::size_t mount_len = LibC::strlen(mp->path);

    if (path[mount_len] == '/')
        ++mount_len;

    if (mount_len >= LibC::strlen(path)) {
        relative_path_out[0] = '\0';
    } else {
        LibC::strlcpy(relative_path_out, path + mount_len, relative_path_out_size);
    }

    return mp->root_vnode;
}

VNode* VFS::lookup(VNode* base, char const* path)
{
    if (FK::alert_if(base == nullptr, "VFS: lookup - base is null"))
        return nullptr;

    if (FK::alert_if(path == nullptr, "VFS: lookup - path is null"))
        return nullptr;

    char segment[MAX_NAME_LEN];
    char const* p = path;

    VNode* current = base;
    current->vnode_ref();

    while (*p != '\0') {
        // Pular múltiplas barras
        while (*p == '/')
            ++p;

        if (*p == '\0')
            break;

        LibC::size_t len = 0;
        while (p[len] != '/' && p[len] != '\0' && len < MAX_NAME_LEN - 1) {
            segment[len] = p[len];
            ++len;
        }
        segment[len] = '\0';
        p += len;

        VNode* next = current->ops->lookup(current, segment);
        if (FK::alert_if(next == nullptr, "VFS: lookup - segment not found")) {
            current->vnode_unref();
            return nullptr;
        }

        current->vnode_unref();
        current = next;
    }

    return current;
}

void VNode::vnode_ref()
{
    __atomic_add_fetch(&ref_count, 1, __ATOMIC_SEQ_CST);
}

void VNode::vnode_unref()
{
    if (__atomic_sub_fetch(&ref_count, 1, __ATOMIC_SEQ_CST) == 0) {
        if (ops && ops->close)
            ops->close(this);
    }
}

VNode* VFS::create_device_node(VNode* parent, char const* name, VNodeType type, void* private_data, VNodeOperations* ops)
{
    if (!parent || !parent->is_directory())
        return nullptr;

    if (!parent->ops || !parent->ops->create)
        return nullptr;

    VNode* vnode = parent->ops->create(parent, name, type);
    if (!vnode)
        return nullptr;

    vnode->stat.type = type;
    vnode->private_data = private_data;
    vnode->ops = ops;

    vnode->stat.permissions = 0660;

    return vnode;
}

}
