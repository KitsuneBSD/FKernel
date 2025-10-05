#pragma once 

#include <Kernel/FileSystem/Vfs/vfs_ops.h>
#include <Kernel/FileSystem/file_types.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/own_ptr.h>

struct VFSFilesystem;

// NOTE: We need turn this code [RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) 
struct VFSNode {
    fixed_string<VFS_MAX_PATH> name;
    FileType type{FileType::Unknown};
    FilePermissions perm{};
    size_t size{0};

    VFSOps* ops{nullptr};
    VFSNode* parent{nullptr};
    static_vector<OwnPtr<VFSNode>, 1024> children;

    void* fs_data{nullptr};
    VFSFilesystem* mounted_fs{nullptr};

    uint32_t refcount{1};

    VFSNode(const char* n,
            FileType t = FileType::Unknown,
            FilePermissions p = {7, 5, 5},
            VFSOps* o = nullptr,
            void* data = nullptr,
            VFSNode* parent_node = nullptr)
        : name(n), type(t), perm(p), ops(o), parent(parent_node), fs_data(data) {}

    void retain() { ++refcount; }
    void release() {
        if (--refcount == 0)
            delete this;
    }

    VFSNode* add_child(OwnPtr<VFSNode> child) {
        child->parent = this;
        children.push_back(move(child));
        return children.back().ptr();
    }
};