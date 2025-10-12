#pragma once

#pragma once
#include <Kernel/FileSystem/VirtualFS/vnode.h>
#include <Kernel/FileSystem/VirtualFS/vnode_ops.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>

struct RamFile
{
    char name[256];
    uint8_t data[1024];
    size_t size{0}; // tamanho atual do arquivo
};

struct RamFS
{
private:
    RetainPtr<VNode> m_root;

public:
    RamFS();
    static RamFS &the();
    RetainPtr<VNode> root();

    // Funções de operação para VNodeOps
    static int ramfs_read(VNode *vnode, void *buffer, size_t size, size_t offset);
    static int ramfs_write(VNode *vnode, const void *buffer, size_t size, size_t offset);
    static int ramfs_open(VNode *vnode, int flags);
    static int ramfs_close(VNode *vnode);
    static int ramfs_lookup(VNode *vnode, const char *name, RetainPtr<VNode> &out);
    static int ramfs_create(VNode *vnode, const char *name, VNodeType type, RetainPtr<VNode> &out);
    static int ramfs_readdir(VNode *vnode, void *buffer, size_t max_entries);
    static int ramfs_unlink(VNode *vnode, const char *name);
    static VNodeOps ops;
};
