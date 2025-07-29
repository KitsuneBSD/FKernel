#include <Kernel/FileSystem/DevFS/DevFS.h>
#include <Kernel/FileSystem/DevFS/DevFSOperations.h>
#include <Kernel/FileSystem/DevFS/DevFSTypes.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/string.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>
#include <LibFK/vector.h>

namespace FileSystem {

static DevFSNode* devfs_root = nullptr;

VNodeOperations devfs_operations = {
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .close = devfs_close,
    .stat = devfs_stat,
    .readdir = devfs_readdir,
    .lookup = devfs_lookup,
    .mkdir = devfs_mkdir,
    .unlink = devfs_unlink,
    .create = devfs_create,
    .rename = devfs_rename,
    .symlink = devfs_symlink,
    .readlink = devfs_readlink,
    .chmod = devfs_chmod,
    .chown = devfs_chown,
};

static bool name_exists(DevFSNode* parent, char const* name)
{
    for (auto& child : parent->children) {
        if (LibC::strcmp(child->name, name) == 0)
            return true;
    }
    return false;
}

VNode* devfs_create_root()
{
    if (devfs_root != nullptr)
        return &devfs_root->vnode;

    void* mem = Falloc(sizeof(DevFSNode));
    if (FK::alert_if(!mem, "devfs: failed to allocate memory for devfsNode"))
        return nullptr;

    LibC::memset(mem, 0, sizeof(DevFSNode));
    devfs_root = new (mem) DevFSNode();

    devfs_root->is_directory = true;
    LibC::strncpy(devfs_root->name, "/", MAX_NAME_LEN);

    devfs_root->vnode.ops = &devfs_operations;
    devfs_root->vnode.private_data = devfs_root;
    devfs_root->vnode.ref_count = 1;
    devfs_root->vnode.stat.type = VNodeType::Directory;
    devfs_root->vnode.stat.size = 0;

    return &devfs_root->vnode;
}

VNode* devfs_init()
{
    Log(LogLevel::INFO, "DevFS: initialized successfully");
    return devfs_create_root();
}

bool devfs_register_device(char const* base_name, VNode* vnode)
{
    if (FK::alert_if(base_name == nullptr, "DevFS: base_name cannot be null"))
        return false;

    if (FK::alert_if(vnode == nullptr, "DevFS: vnode cannot be null"))
        return false;

    FK::enforcef(devfs_root != nullptr, "DevFS: root node not initialized");

    char new_name[256];
    LibC::size_t base_len = LibC::strlen(base_name);
    FK::enforcef(base_len < sizeof(new_name), "DevFS: base name too long");

    if (!name_exists(devfs_root, base_name)) {
        LibC::strcpy(new_name, base_name);
    } else {
        bool found = false;
        for (int i = 0; i < 1000; ++i) {
            int written = LibC::snprintf(new_name, sizeof(new_name), "%s%d", base_name, i);
            if (written < 0 || (LibC::size_t)written >= sizeof(new_name))
                return false;

            if (!name_exists(devfs_root, new_name)) {
                found = true;
                break;
            }
        }
        if (!found) {
            FK::alert("DevFS: unable to find free name for device");
            return false;
        }
    }

    void* mem = Falloc(sizeof(DevFSNode));
    if (FK::alert_if(!mem, "devfs: failed to allocate memory for device node"))
        return false;

    LibC::memset(mem, 0, sizeof(DevFSNode));
    auto* node = new (mem) DevFSNode();

    LibC::strncpy(node->name, new_name, MAX_NAME_LEN);
    node->device_vnode = vnode;
    node->parent = devfs_root;
    node->is_directory = false;

    node->vnode.ops = vnode->ops;
    node->vnode.private_data = vnode->private_data;
    node->vnode.ref_count = 1;
    node->vnode.stat = vnode->stat;

    devfs_root->children.push_back(node);

    Logf(LogLevel::INFO, "DevFS: registered device '%s'", new_name);

    return true;
}

int devfs_create_node(VNode* vnode, char const* name, VNodeType type)
{
    if (!vnode || !name)
        return -1;

    auto* parent_node = reinterpret_cast<DevFSNode*>(vnode->private_data);
    if (!parent_node || !parent_node->is_directory)
        return -1;

    for (auto& child : parent_node->children) {
        if (LibC::strcmp(child->name, name) == 0)
            return -1;
    }

    void* mem = Falloc(sizeof(DevFSNode));
    if (!mem)
        return -1;

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
    return 0;
}

}
