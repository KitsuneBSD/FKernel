#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/intrusiveList.h>

namespace FileSystem {

struct RamFSNode {
    VNode vnode;

    char name[MAX_NAME_LEN];
    char symlink_target[MAX_SYMLINK_TARGET_LEN];

    LibC::uint8_t* data;
    LibC::size_t data_size;
    LibC::size_t data_capacity;

    FK::IntrusiveNode<RamFSNode> child_node;
    FK::IntrusiveList<RamFSNode, &RamFSNode::child_node> children;

    RamFSNode* parent;
    bool is_directory;
};

}
