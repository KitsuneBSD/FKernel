#include <Kernel/FileSystem/RamFS/RamFS.h>
#include <Kernel/FileSystem/RamFS/RamFSOperations.h>
#include <Kernel/FileSystem/RamFS/RamFSTypes.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>
#include <LibFK/new.h>

namespace FileSystem {

VNodeOperations ramfs_operations = {
    .open = ramfs_open,
    .read = ramfs_read,
    .write = ramfs_write,
    .close = ramfs_close,
    .stat = ramfs_stat,
    .readdir = ramfs_readdir,
    .lookup = ramfs_lookup,
    .mkdir = ramfs_mkdir,
    .unlink = ramfs_unlink,
    .create = ramfs_create,
    .rename = ramfs_rename,
    .symlink = ramfs_symlink,
    .readlink = ramfs_readlink,
    .chmod = ramfs_chmod,
    .chown = ramfs_chown,
};

VNode* ramfs_create_root()
{
    void* mem = Falloc(sizeof(RamFSNode));
    if (FK::alert_if(!mem, "RAMFS: failed to allocate memory to RamFSNode"))
        return nullptr;

    LibC::memset(mem, 0, sizeof(RamFSNode));

    auto* node = new (mem) RamFSNode();
    if (FK::alert_if(!node, "RAMFS: placement new for RamFSNode failed"))
        return nullptr;

    node->is_directory = true;

    if (LibC::strncpy(node->name, "/", MAX_NAME_LEN) == nullptr) {
        FK::alert("RAMFS: strncpy failed copying root name");
        return nullptr;
    }

    node->vnode.ops = &ramfs_operations;
    node->vnode.private_data = node;
    node->vnode.ref_count = 1;
    node->vnode.stat.type = VNodeType::Directory;
    node->vnode.stat.size = 0;

    if (FK::alert_if(node->vnode.ops == nullptr, "RAMFS: vnode ops pointer null"))
        return nullptr;
    if (FK::alert_if(node->vnode.private_data == nullptr, "RAMFS: vnode private_data null"))
        return nullptr;

    return &node->vnode;
}

}
