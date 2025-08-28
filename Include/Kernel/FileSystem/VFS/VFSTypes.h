#pragma once

#include <Kernel/FileSystem/VFS/File/FileHandle.h>
#include <Kernel/FileSystem/VFS/File/FileStat.h>
#include <Kernel/FileSystem/VFS/VNodeType.h>
#include <LibC/stddef.h>
#include <LibFK/intrusiveList.h>

namespace FileSystem {

constexpr LibC::size_t MAX_NAME_LEN = 256;
constexpr LibC::size_t MAX_PATH_LEN = 1024;
constexpr LibC::size_t MAX_SYMLINK_TARGET_LEN = 256;

struct VNodeOperations {
    int (*open)(VNode* vnode, LibC::uint32_t flags);
    LibC::ssize_t (*read)(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size);
    LibC::ssize_t (*write)(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size);
    int (*close)(VNode* vnode);
    int (*stat)(VNode* vnode, FileStat* stat);
    int (*readdir)(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode);
    VNode* (*lookup)(VNode* vnode, char const* name);
    int (*mkdir)(VNode* vnode, char const* name, LibC::uint32_t permissions);
    int (*unlink)(VNode* vnode, char const* name);
    VNode* (*create)(VNode* parent, char const* name, VNodeType type);
    int (*rename)(VNode* vnode, char const* oldname, char const* newname);
    int (*symlink)(VNode* vnode, char const* target, char const* linkname);
    LibC::ssize_t (*readlink)(VNode* vnode, char* buf, LibC::size_t bufsize);
    int (*chmod)(VNode* vnode, LibC::uint32_t new_permissions);
    int (*chown)(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid);
};

struct VNode {
    VNodeOperations* ops;
    FileStat stat;
    int volatile ref_count;

    FK::IntrusiveNode<VNode> mount_list_node;
    void* private_data;

    void vnode_ref();
    void vnode_unref();

    bool is_directory() const
    {
        return stat.type == VNodeType::Directory;
    }

    bool is_symbolic() const
    {
        return stat.type == VNodeType::Symlink;
    }

    bool is_file() const
    {
        return stat.type == VNodeType::File;
    }
};

}
