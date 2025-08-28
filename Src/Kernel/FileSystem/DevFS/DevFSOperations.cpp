#include <Kernel/FileSystem/DevFS/DevFS.h>
#include <Kernel/FileSystem/DevFS/DevFSOperations.h>
#include <Kernel/FileSystem/DevFS/DevFSTypes.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/types.h>

namespace FileSystem {

int devfs_open(VNode* vnode, LibC::uint32_t flags)
{
    UNUSED(flags);
    if (!vnode)
        return -1;
    return 0;
}

LibC::ssize_t devfs_read(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size)
{
    UNUSED(offset);
    UNUSED(size);

    if (!vnode || !buffer)
        return -1;

    return 0;
}

LibC::ssize_t devfs_write(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size)
{
    UNUSED(offset);
    UNUSED(size);

    if (!vnode || !buffer)
        return -1;

    return 0;
}

int devfs_close(VNode* vnode)
{
    if (!vnode)
        return -1;
    return 0;
}

int devfs_stat(VNode* vnode, FileStat* statbuf)
{
    if (!vnode || !statbuf)
        return -1;

    *statbuf = vnode->stat;
    return 0;
}

int devfs_readdir(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode)
{
    if (!vnode || !name_out || !out_vnode)
        return -1;

    auto* node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!node || !node->is_directory)
        return -1;

    if (index >= node->children.size())
        return -1;

    DevFSNode* child = node->children[index];
    LibC::strncpy(name_out, child->name, MAX_NAME_LEN);
    *out_vnode = &child->vnode;
    return 0;
}

VNode* devfs_lookup(VNode* vnode, char const* name)
{
    if (!vnode || !name)
        return nullptr;

    auto* node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!node || !node->is_directory)
        return nullptr;

    for (auto& child : node->children) {
        if (LibC::strcmp(child->name, name) == 0)
            return &child->vnode;
    }
    return nullptr;
}

int devfs_mkdir(VNode* vnode, char const* name, LibC::uint32_t permissions)
{
    UNUSED(permissions);
    return devfs_create_node(vnode, name, VNodeType::Directory);
}

VNode* devfs_create(VNode* parent, char const* name, VNodeType type)
{
    if (!parent || !name)
        return nullptr;

    auto* parent_node = reinterpret_cast<DevFSNode*>(parent->private_data);
    if (!parent_node || !parent_node->is_directory)
        return nullptr;

    for (auto& child : parent_node->children) {
        if (LibC::strcmp(child->name, name) == 0)
            return nullptr;
    }

    void* mem = Falloc(sizeof(DevFSNode));
    if (!mem)
        return nullptr;

    LibC::memset(mem, 0, sizeof(DevFSNode));
    auto* new_node = new (mem) DevFSNode();

    LibC::strncpy(new_node->name, name, MAX_NAME_LEN);
    new_node->is_directory = (type == VNodeType::Directory);
    new_node->parent = parent_node;

    new_node->vnode.ops = &devfs_operations;
    new_node->vnode.private_data = new_node;
    new_node->vnode.ref_count = 1;
    new_node->vnode.stat.type = type;
    new_node->vnode.stat.size = 0;

    parent_node->children.push_back(new_node);
    return &new_node->vnode;
}

int devfs_symlink(VNode* vnode, char const* target, char const* linkname)
{
    if (!vnode || !target || !linkname)
        return -1;

    auto* parent_node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!parent_node || !parent_node->is_directory)
        return -1;

    for (auto& child : parent_node->children) {
        if (LibC::strcmp(child->name, linkname) == 0)
            return -1;
    }

    void* mem = Falloc(sizeof(DevFSNode));
    if (!mem)
        return -1;

    LibC::memset(mem, 0, sizeof(DevFSNode));
    auto* new_node = new (mem) DevFSNode();

    LibC::strncpy(new_node->name, linkname, MAX_NAME_LEN);
    new_node->is_directory = false;
    new_node->parent = parent_node;

    new_node->vnode.ops = &devfs_operations;
    new_node->vnode.private_data = new_node;
    new_node->vnode.ref_count = 1;
    new_node->vnode.stat.type = VNodeType::Symlink;
    new_node->vnode.stat.size = LibC::strlen(target);

    parent_node->children.push_back(new_node);
    return 0;
}

LibC::ssize_t devfs_readlink(VNode* vnode, char* buf, LibC::size_t bufsize)
{
    UNUSED(vnode);
    UNUSED(buf);
    UNUSED(bufsize);
    return -1;
}

int devfs_unlink(VNode* vnode, char const* name)
{
    if (!vnode || !name)
        return -1;

    auto* parent_node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!parent_node || !parent_node->is_directory)
        return -1;

    for (LibC::size_t i = 0; i < parent_node->children.size(); ++i) {
        DevFSNode* child = parent_node->children[i];
        if (LibC::strcmp(child->name, name) == 0) {
            parent_node->children.remove_at(i);
            child->~DevFSNode();
            Ffree(child);
            return 0;
        }
    }
    return -1;
}

int devfs_rename(VNode* vnode, char const* oldname, char const* newname)
{
    if (!vnode || !oldname || !newname)
        return -1;

    auto* parent_node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!parent_node || !parent_node->is_directory)
        return -1;

    for (auto& child : parent_node->children) {
        if (LibC::strcmp(child->name, newname) == 0)
            return -1;
    }

    for (auto& child : parent_node->children) {
        if (LibC::strcmp(child->name, oldname) == 0) {
            LibC::strncpy(child->name, newname, MAX_NAME_LEN);
            return 0;
        }
    }
    return -1;
}

int devfs_chmod(VNode* vnode, LibC::uint32_t new_permissions)
{
    if (!vnode)
        return -1;

    vnode->stat.permissions = new_permissions;
    return 0;
}

int devfs_chown(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid)
{
    if (!vnode)
        return -1;

    vnode->stat.uid = new_uid;
    vnode->stat.gid = new_gid;
    return 0;
}

} // namespace FileSystem
