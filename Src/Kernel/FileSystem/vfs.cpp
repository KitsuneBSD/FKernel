#include <Kernel/FileSystem/vfs.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>
#include <LibFK/new.h>

// no global OwnPtr, just a raw pointer
static VFSNode* g_root_raw = nullptr;

VFSNode* VFS::root() {
    return g_root_raw;
}

void VFS::init() {
    static bool initialized = false;
    if (initialized) return;

    FilePermissions perms{7, 5, 5}; // rwx for user, r-x for group/others
    g_root_raw = new VFSNode("/", FileType::Directory, perms);

    klog("VFS", "Initialized root directory");

    initialized = true;
}

static VFSNode* create_node(const char* name, FileType type, FilePermissions perms) {
    auto node = new VFSNode(name, type, perms);
    return node;
}

static VFSNode* lookup_child(VFSNode* parent, const char* name) {
    for (auto& child : parent->children) {
        if (strcmp(child->name.c_str(), name) == 0)
            return child.ptr();
    }
    return nullptr;
}

VFSNode* VFS::resolve_path(const char* path) {
    if (!root()) return nullptr;
    if (strcmp(path, "/") == 0) return root();

    VFSNode* node = root();
    char buf[VFS_MAX_PATH];
    strncpy(buf, path, VFS_MAX_PATH);

    char* token = strtok(buf, "/");
    while (token) {
        if (node->type != FileType::Directory) return nullptr;
        node = lookup_child(node, token);
        if (!node) return nullptr;
        token = strtok(nullptr, "/");
    }
    return node;
}

int VFS::mkdir(const char* path, FilePermissions perms) {
    if (!root()) return -1;

    char buf[VFS_MAX_PATH];
    strncpy(buf, path, VFS_MAX_PATH);
    char* last_slash = strrchr(buf, '/');
    char* name = nullptr;
    VFSNode* parent = nullptr;

    if (!last_slash) return -1;

    if (last_slash == buf) {
        parent = root();
        name = last_slash + 1;
    } else {
        *last_slash = '\0';
        parent = resolve_path(buf);
        name = last_slash + 1;
    }

    if (!parent || parent->type != FileType::Directory) return -1;
    if (lookup_child(parent, name)) return -1;

    auto node = create_node(name, FileType::Directory, perms);
    node->parent = parent;
    parent->children.push_back(OwnPtr<VFSNode>(node));
    return 0;
}

int VFS::unlink(const char* path) {
    auto node = resolve_path(path);
    if (!node || node == root()) return -1;

    auto parent = node->parent;
    if (!parent) return -1;

    for (size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i].ptr() == node) {
            parent->children.erase(i); // OwnPtr destructor frees node
            return 0;
        }
    }
    return -1;
}

ssize_t VFS::read(VFSNode* node, void* buf, size_t size, size_t offset) {
    if (!node || node->type != FileType::Regular) return -1;
    if (!node->ops) return -1;
    return node->ops->read(node, buf, size, offset);
}

ssize_t VFS::write(VFSNode* node, const void* buf, size_t size, size_t offset) {
    if (!node || node->type != FileType::Regular) return -1;
    if (!node->ops) return -1;
    return node->ops->write(node, buf, size, offset);
}

int VFS::open(VFSNode* node, FileMode mode) {
    if (!node || !node->ops) return -1;
    return node->ops->open(node, mode);
}

int VFS::close(VFSNode* node) {
    if (!node || !node->ops) return -1;
    return node->ops->close(node);
}

int VFS::mount(VFSNode* node, const char* path) {
    if (strcmp(path, "/") != 0) return -1;
    g_root_raw = node; // just assign raw pointer
    return 0;
}
