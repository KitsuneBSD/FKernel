#include <Kernel/FileSystem/RamFS/RamFSOperations.h>
#include <Kernel/FileSystem/RamFS/RamFSTypes.h>
#include <Kernel/FileSystem/RamFS/RamFSUtils.h>
#include <Kernel/FileSystem/VFS/FilePermissions.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/Posix/errno.h>
#include <LibC/stddef.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>
#include <LibFK/new.h>
#include <LibFK/types.h>
#include <LibFK/utils.h>

namespace FileSystem {

inline RamFSNode* vnode_to_ramfsnode(VNode* vnode)
{
    return container_of(vnode, RamFSNode, vnode);
}

int ramfs_open(VNode* vnode, LibC::uint32_t flags)
{
    if (!vnode) {
        Logf(LogLevel::TRACE, "ramfs_open: vnode is null");
        return -EINVAL;
    }

    Logf(LogLevel::TRACE, "ramfs_open: vnode=%s flags=0x%x", vnode_to_ramfsnode(vnode)->name, flags);

    if (vnode->stat.type == VNodeType::Directory) {
        Logf(LogLevel::TRACE, "ramfs_open: cannot open directory as file");
        return -EISDIR;
    }

    bool write_mode = (flags & O_WRONLY) || (flags & O_RDWR);
    bool do_trunc = (flags & O_TRUNC) != 0;

    if (do_trunc && write_mode) {
        RamFSNode* node = vnode_to_ramfsnode(vnode);
        if (node->data) {
            Ffree(node->data);
            node->data = nullptr;
        }
        node->data_size = 0;
        vnode->stat.size = 0;

        Logf(LogLevel::TRACE, "ramfs_open: file truncated");
    }

    return 0;
}

LibC::ssize_t ramfs_read(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);
    Logf(LogLevel::TRACE, "ramfs_read called: vnode=%s, offset=%llu, size=%zu", node->name, offset, size);

    if (offset >= node->data_size) {
        Logf(LogLevel::TRACE, "ramfs_read: offset beyond data_size");
        return 0;
    }

    LibC::size_t to_read = size;
    if (offset + to_read > node->data_size)
        to_read = node->data_size - offset;

    LibC::memcpy(buffer, node->data + offset, to_read);
    return static_cast<LibC::ssize_t>(to_read);
}

LibC::ssize_t ramfs_write(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);
    Logf(LogLevel::TRACE, "ramfs_write called: vnode=%s, offset=%llu, size=%zu", node->name, offset, size);

    LibC::size_t needed = offset + size;
    if (!ramfs_node_expand_data(node, needed)) {
        Logf(LogLevel::TRACE, "ramfs_write: failed to expand data");
        return -ENOMEM;
    }

    LibC::memcpy(node->data + offset, buffer, size);

    if (needed > node->data_size)
        node->data_size = needed;

    vnode->stat.size = node->data_size;
    return static_cast<LibC::ssize_t>(size);
}

int ramfs_close(VNode* vnode)
{
    UNUSED(vnode)
    Logf(LogLevel::TRACE, "ramfs_close called");
    return 0;
}

int ramfs_stat(VNode* vnode, FileStat* stat)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);

    Logf(LogLevel::TRACE, "ramfs_stat called: vnode=%s", node->name);

    stat->size = node->data_size;
    stat->permissions = vnode->stat.permissions;
    stat->inode = vnode->stat.inode;
    stat->type = vnode->stat.type;
    return 0;
}

int ramfs_readdir(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);

    if (!node->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_readdir: vnode=%s is not a directory", node->name);
        return -ENOTDIR;
    }

    LibC::size_t i = 0;
    for (auto& child : node->children) {
        if (i == index) {
            Logf(LogLevel::TRACE, "ramfs_readdir: found child at index %llu -> %s", index, child.name);
            LibC::strncpy(name_out, child.name, MAX_NAME_LEN);
            *out_vnode = &child.vnode;
            return 0;
        }
        i++;
    }

    Logf(LogLevel::TRACE, "ramfs_readdir: no child at index %llu", index);
    return -ENOENT;
}

VNode* ramfs_create(VNode* parent, char const* name, VNodeType type)
{
    RamFSNode* parent_node = vnode_to_ramfsnode(parent);

    if (!parent_node->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_create: parent vnode=%s is not a directory", parent_node->name);
        return nullptr;
    }

    Logf(LogLevel::TRACE, "ramfs_create: creating node '%s' of type %d under '%s'", name, static_cast<int>(type), parent_node->name);

    void* mem = Falloc(sizeof(RamFSNode));
    if (FK::alert_if(!mem, "RAMFS: failed to allocate memory for new node"))
        return nullptr;

    LibC::memset(mem, 0, sizeof(RamFSNode));
    auto* node = new (mem) RamFSNode();
    node->is_directory = (type == VNodeType::Directory);
    node->data_size = 0;
    node->data_capacity = 0;
    node->data = nullptr;
    LibC::strncpy(node->name, name, MAX_NAME_LEN);
    node->name[MAX_NAME_LEN - 1] = '\0';

    node->vnode.ops = &ramfs_operations;
    node->vnode.private_data = node;
    node->vnode.ref_count = 1;
    node->vnode.stat.type = type;
    node->vnode.stat.size = 0;

    node->parent = parent_node;
    parent_node->children.append(node);

    return &node->vnode;
}

int ramfs_mkdir(VNode* vnode, char const* name, LibC::uint32_t permissions)
{
    RamFSNode* dir = vnode_to_ramfsnode(vnode);

    if (!dir->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_mkdir: vnode=%s is not a directory", dir->name);
        return -ENOTDIR;
    }

    Logf(LogLevel::TRACE, "ramfs_mkdir: creating directory '%s' under '%s'", name, dir->name);

    for (auto& child : dir->children) {
        if (LibC::strcmp(child.name, name) == 0) {
            Logf(LogLevel::TRACE, "ramfs_mkdir: directory '%s' already exists", name);
            return -EEXIST;
        }
    }

    void* mem = Falloc(sizeof(RamFSNode));
    if (!mem) {
        Logf(LogLevel::TRACE, "ramfs_mkdir: failed to allocate memory for new directory");
        return -ENOMEM;
    }

    LibC::memset(mem, 0, sizeof(RamFSNode));
    auto* new_node = new (mem) RamFSNode();
    new_node->is_directory = true;
    LibC::strncpy(new_node->name, name, MAX_NAME_LEN);
    new_node->name[MAX_NAME_LEN - 1] = '\0';

    new_node->vnode.ops = &ramfs_operations;
    new_node->vnode.private_data = new_node;
    new_node->vnode.ref_count = 1;
    new_node->vnode.stat.type = VNodeType::Directory;
    new_node->vnode.stat.permissions = permissions;

    new_node->parent = dir;
    dir->children.append(new_node);

    return 0;
}

int ramfs_unlink(VNode* parent, char const* name)
{
    RamFSNode* parent_node = vnode_to_ramfsnode(parent);
    if (!parent_node->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_unlink: vnode=%s is not a directory", parent_node->name);
        return -ENOTDIR;
    }

    Logf(LogLevel::TRACE, "ramfs_unlink: removing '%s' from '%s'", name, parent_node->name);

    for (auto it = parent_node->children.begin(); it != parent_node->children.end(); ++it) {
        if (LibC::strcmp(it->name, name) == 0) {
            parent_node->children.erase(it);
            return 0;
        }
    }

    Logf(LogLevel::TRACE, "ramfs_unlink: '%s' not found in '%s'", name, parent_node->name);
    return -ENOENT;
}

int ramfs_rename(VNode* vnode, char const* oldname, char const* newname)
{
    RamFSNode* dir = vnode_to_ramfsnode(vnode);
    if (!dir->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_rename: vnode=%s is not a directory", dir->name);
        return -ENOTDIR;
    }

    Logf(LogLevel::TRACE, "ramfs_rename: renaming '%s' to '%s' in directory '%s'", oldname, newname, dir->name);

    for (auto& child : dir->children) {
        if (LibC::strcmp(child.name, oldname) == 0) {
            LibC::strncpy(child.name, newname, MAX_NAME_LEN);
            child.name[MAX_NAME_LEN - 1] = '\0';
            return 0;
        }
    }
    Logf(LogLevel::TRACE, "ramfs_rename: '%s' not found in '%s'", oldname, dir->name);
    return -ENOENT;
}

int ramfs_symlink(VNode* vnode, char const* target, char const* linkname)
{
    if (!vnode || !vnode->ops || !vnode->ops->create) {
        Logf(LogLevel::TRACE, "ramfs_symlink: invalid vnode or ops");
        return -EINVAL;
    }

    VNode* link_vnode = vnode->ops->create(vnode, linkname, VNodeType::Symlink);
    if (!link_vnode) {
        Logf(LogLevel::TRACE, "ramfs_symlink: failed to create symlink vnode");
        return -ENOMEM;
    }

    RamFSNode* link_node = vnode_to_ramfsnode(link_vnode);
    if (!link_node) {
        Logf(LogLevel::TRACE, "ramfs_symlink: failed to convert vnode to RamFSNode");
        return -EFAULT;
    }

    LibC::strncpy(link_node->symlink_target, target, MAX_SYMLINK_TARGET_LEN - 1);
    link_node->symlink_target[MAX_SYMLINK_TARGET_LEN - 1] = '\0';

    Logf(LogLevel::TRACE, "ramfs_symlink: created symlink '%s' -> '%s'", linkname, target);
    return 0;
}

LibC::ssize_t ramfs_readlink(VNode* vnode, char* buf, LibC::size_t bufsize)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);

    if (node->vnode.stat.type != VNodeType::Symlink) {
        Logf(LogLevel::TRACE, "ramfs_readlink: vnode is not a symlink");
        return -EINVAL;
    }

    LibC::size_t len = LibC::strlen(node->symlink_target);
    LibC::size_t to_copy = LibFK::min(len, bufsize);

    LibC::memcpy(buf, node->symlink_target, to_copy);
    if (len > bufsize)
        Logf(LogLevel::TRACE, "ramfs_readlink: truncated symlink ({} > {})", len, bufsize);

    return static_cast<LibC::ssize_t>(to_copy);
}

int ramfs_chown(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid)
{
    vnode->stat.uid = new_uid;
    vnode->stat.gid = new_gid;
    Logf(LogLevel::TRACE, "ramfs_chown: changed uid=%u gid=%u", new_uid, new_gid);
    return 0;
}

int ramfs_chmod(VNode* vnode, LibC::uint32_t new_permissions)
{
    vnode->stat.permissions = new_permissions;
    Logf(LogLevel::TRACE, "ramfs_chmod: changed permissions to %u", new_permissions);
    return 0;
}

VNode* ramfs_lookup(VNode* vnode, char const* name)
{
    RamFSNode* node = vnode_to_ramfsnode(vnode);

    if (!node->is_directory) {
        Logf(LogLevel::TRACE, "ramfs_lookup: Node '%s' is not a directory", node->name);
        return nullptr;
    }

    Logf(LogLevel::TRACE, "ramfs_lookup: directory=%s, target=%s", node->name, name);

    for (auto& child : node->children) {
        Logf(LogLevel::TRACE, "ramfs_lookup: Checking child '%s'", child.name);
        if (LibC::strcmp(child.name, name) == 0) {
            Logf(LogLevel::TRACE, "ramfs_lookup %s", child.name);
            return &child.vnode;
        }
    }

    Logf(LogLevel::TRACE, "ramfs_lookup: Child %s not found in directory %s", name, node->name);
    return nullptr;
}

}
