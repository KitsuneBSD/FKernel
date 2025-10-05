#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/DevFS/devfs_ops.h>

#include <LibC/string.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/own_ptr.h>

VFSNode* g_devfs_root = nullptr;

void devfs_init() {
    if (g_devfs_root) return;

    g_devfs_root = new VFSNode("/dev", FileType::Directory, {7,5,5}, new DevFSOps(), nullptr, nullptr);
    klog("DevFS", "Initialized /dev");
}

VFSNode* DevFSOps::lookup(VFSNode* node, const char* name) {
    for (auto& child : node->children) {
        if (strcmp(child->name.c_str(), name) == 0)
            return child.ptr();
    }
    return nullptr;
}

ssize_t DevFSOps::read(VFSNode* node, void* buf, size_t size, size_t offset) {
    (void)node; (void)buf; (void)size; (void)offset;
    return -ENOSYS;
}

ssize_t DevFSOps::write(VFSNode* node, const void* buf, size_t size, size_t offset) {
    (void)node; (void)buf; (void)size; (void)offset;
    return -ENOSYS;
}

VFSNode* devfs_register(const char* name, FileType device_type , VFSOps* ops, void* dev_data) {
    ASSERT(device_type == FileType::BlockDevice || device_type == FileType::CharDevice);

    auto node = new VFSNode(name, device_type, {6,4,4}, ops, dev_data, g_devfs_root);
    g_devfs_root->children.push_back(OwnPtr<VFSNode>(node));
    klog("DevFS", "Registered device: %s", name);
    return node;
}

void devfs_unregister(const char* name) {
    if (!g_devfs_root) return;

    for (size_t i = 0; i < g_devfs_root->children.size(); ++i) {
        if (strcmp(g_devfs_root->children[i]->name.c_str(), name) == 0) {
            g_devfs_root->children.erase(i);
            klog("DevFS", "Unregistered device: %s", name);
            return;
        }
    }
}