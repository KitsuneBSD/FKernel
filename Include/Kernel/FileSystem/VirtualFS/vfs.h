#pragma once

#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_flags.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <Kernel/FileSystem/VirtualFS/vnode_type.h>

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
    RetainPtr<VNode> v_root;
    RetainPtr<VNode> v_cwd;
    // TODO: Apply b+ tree instead static_vector
    static_vector<Mountpoint, 8> v_mounts;

    RetainPtr<VNode> resolve_path(const char *path, RetainPtr<VNode> cwd);
    void set_cwd(RetainPtr<VNode> vnode);

public:
    static VirtualFS &the()
    {
        static VirtualFS inst;
        return inst;
    }
    int mount(const char *name, RetainPtr<VNode> root);
    int lookup(const char *path, RetainPtr<VNode> &out);
    int open(const char *path, int flags);
    int read(int file_descriptor, void *buf, size_t sz, size_t off);
    int write(int file_descriptor, const void *buf, size_t sz, size_t off);

    RetainPtr<VNode> root() const;
    RetainPtr<VNode> cwd() const;
};