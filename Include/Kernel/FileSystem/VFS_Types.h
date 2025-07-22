#pragma once

#include <LibC/stddef.h>
#include <LibFK/intrusiveList.h>

namespace FileSystem {

constexpr LibC::size_t MAX_NAME_LEN = 256;
constexpr LibC::size_t MAX_PATH_LEN = 1024;

struct VNode;

enum class VNodeType : LibC::uint8_t {
    Unknown,
    File,
    Directory,
    Symlink,
    Device,
    Mountpoint
};

struct MountPoint {
    char path[MAX_PATH_LEN];
    VNode* root_vnode;
    FK::IntrusiveNode<MountPoint> list_node;
};

struct FileStat {
    LibC::uint64_t size;
    LibC::uint32_t permissions;
    LibC::uint64_t inode;
    VNodeType type;
};

struct VNodeOperations {
    int (*open)(VNode* vnode, LibC::uint32_t flags);
    LibC::ssize_t (*read)(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size);
    LibC::ssize_t (*write)(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size);
    int (*close)(VNode* vnode);
    int (*stat)(VNode* vnode, FileStat* stat);
    int (*readdir)(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode);
    VNode* (*lookup)(VNode* vnode, char const* name);
};

struct VNode {
    VNodeOperations* ops;
    FileStat stat;
    int volatile ref_count;

    FK::IntrusiveNode<VNode> mount_list_node;
    void* private_data;

    void vnode_ref();
    void vnode_unref();
};
struct FileHandle {
    VNode* vnode;
    LibC::uint64_t offset;
    LibC::uint32_t flags;

    LibC::ssize_t filehandle_read(void* buf, LibC::size_t size);
    LibC::ssize_t filehandle_write(void const* buf, LibC::size_t size);
};

}
