#include <Kernel/FileSystem/VFS_Types.h>
#include <Kernel/FileSystem/VirtualFileSystem.h>
#include <Kernel/Posix/errno.h>
#include <LibC/string.h>
#include <LibFK/intrusiveList.h>

namespace FileSystem {

static FK::IntrusiveList<MountPoint, &MountPoint::list_node> g_mount_points;

void VNode::vnode_ref()
{
    __atomic_fetch_add(&ref_count, 1, __ATOMIC_RELAXED);
}

void VNode::vnode_unref()
{
    auto prev = __atomic_fetch_sub(&ref_count, 1, __ATOMIC_ACQ_REL);
    if (FK::alert_if_f(prev > 0, "VNode_unref called on VNode with zero ref_count"))
        return;
    if (prev == 1) {
        // TODO: liberar recursos privados da VNode (private_data) se necessário
        // e liberar a própria VNode, se for gerenciada dinamicamente.
    }
}

LibC::ssize_t FileHandle::filehandle_read(void* buf, LibC::size_t size)
{
    if (!vnode || !vnode->ops || !vnode->ops->read)
        return -EINVAL;

    auto ret = vnode->ops->read(vnode, offset, buf, size);
    if (ret > 0)
        offset += ret;
    return ret;
}

LibC::ssize_t FileHandle::filehandle_write(void const* buf, LibC::size_t size)
{
    if (!vnode || !vnode->ops || !vnode->ops->write)
        return EINVAL;

    auto ret = vnode->ops->write(vnode, offset, buf, size);
    if (ret > 0)
        offset += ret;
    return ret;
}

void register_mount_point(MountPoint* mp)
{
    if (FK::alert_if_f(mp == nullptr, "MountPoint can't be null"))
        return;
    g_mount_points.append(mp);
}

void unregister_mount_point(MountPoint* mp)
{
    if (FK::alert_if_f(mp == nullptr, "MountPoint can't be null"))
        return;
    g_mount_points.remove(mp);
}

MountPoint* find_mount_point_for_path(char const* path)
{
    MountPoint* found = nullptr;
    LibC::size_t longest_match = 0;

    g_mount_points.for_each([&](MountPoint* mp) {
        LibC::size_t len = LibC::strlen(mp->path);
        if (len <= longest_match)
            return;

        if (LibC::strncmp(path, mp->path, len) == 0) {
            found = mp;
            longest_match = len;
        }
    });

    return found;
}

VNode* lookup_path(char const* path)
{
    if (!path || path[0] == '\0')
        return nullptr;

    auto mp = find_mount_point_for_path(path);
    if (!mp)
        return nullptr;

    LibC::size_t mount_len = LibC::strlen(mp->path);
    char const* sub_path = path + mount_len;

    if (sub_path[0] == '\0' || sub_path[0] == '/')
        return mp->root_vnode;

    VNode* current = mp->root_vnode;
    if (!current || !current->ops || !current->ops->lookup)
        return nullptr;

    char name_buf[MAX_NAME_LEN];
    char const* p = sub_path;
    while (*p == '/')
        ++p;

    while (*p && current) {
        LibC::size_t i = 0;
        while (p[i] && p[i] != '/' && i < MAX_NAME_LEN - 1) {
            name_buf[i] = p[i];
            i++;
        }
        name_buf[i] = '\0';

        current = current->ops->lookup(current, name_buf);

        if (!current)
            return nullptr;

        p += i;
        while (*p == '/')
            ++p;
    }

    return current;
}

int open_path(char const* path, LibC::uint32_t flags, FileHandle* out_handle)
{
    if (!path || !out_handle)
        return EINVAL;

    VNode* VNode = lookup_path(path);
    if (!VNode)
        return ENOENT;

    if (!VNode->ops || !VNode->ops->open)
        return ENOSYS;

    int err = VNode->ops->open(VNode, flags);
    if (err < 0)
        return err;

    VNode->vnode_ref();

    out_handle->vnode = VNode;
    out_handle->offset = 0;
    out_handle->flags = flags;

    return 0;
}

int close_filehandle(FileHandle* handle)
{
    if (!handle || !handle->vnode)
        return EINVAL;

    if (handle->vnode->ops && handle->vnode->ops->close)
        handle->vnode->ops->close(handle->vnode);

    handle->vnode->vnode_unref();
    handle->vnode = nullptr;
    handle->offset = 0;
    handle->flags = 0;

    return 0;
}

}
