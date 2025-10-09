#pragma once

#include <Kernel/FileSystem/VirtualFS/VNode.h>
#include <Kernel/FileSystem/VirtualFS/VNodeFlags.h>
#include <Kernel/FileSystem/VirtualFS/VNodeOps.h>
#include <Kernel/FileSystem/VirtualFS/VNodeType.h>

struct Mountpoint
{
    fixed_string<256> m_name;
    RetainPtr<VNode> m_root;
    void *m_fs_private{nullptr};

    Mountpoint() = default;
    Mountpoint(const char *name, RetainPtr<VNode> r) : m_name(name), m_root(r) {}
};

class VirtualFS
{
private:
    VirtualFS() = default;
    RetainPtr<VNode> m_root;
    // TODO: Apply b+ tree instead static_vector
    static_vector<Mountpoint, 8> m_mounts;
    VNode* resolve_path(const char *path);

public:
    static VirtualFS &the()
    {
        static VirtualFS* fs = new VirtualFS();
        return *fs;
    }

    int mount(const char *name, RetainPtr<VNode> root);
    int lookup(const char *path, RetainPtr<VNode> &out);
    int open(const char *path, int flags, RetainPtr<VNode> &out);
    int read(const char *path, void *buf, size_t sz, size_t off);
    int write(const char *path, const void *buf, size_t sz, size_t off);

    RetainPtr<VNode> root();
};