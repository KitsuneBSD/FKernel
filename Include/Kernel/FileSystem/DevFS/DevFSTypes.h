#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibFK/vector.h>

namespace FileSystem {

struct VNode;

struct DevFSNode {
    VNode vnode;
    char name[MAX_NAME_LEN];
    VNode* device_vnode;

    FK::Vector<DevFSNode*> children;

    DevFSNode* parent;
    bool is_directory;
};
}
