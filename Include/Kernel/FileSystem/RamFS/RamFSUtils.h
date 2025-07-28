#pragma once

#include <Kernel/FileSystem/RamFS/RamFSTypes.h>
#include <LibC/stddef.h>

namespace FileSystem {
bool ramfs_node_expand_data(RamFSNode* node, LibC::size_t new_size);
void ramfs_node_free_data(RamFSNode* node);
void ramfs_add_child(RamFSNode* parent, RamFSNode* child);
RamFSNode* vnode_to_ramfsnode(VNode* vnode);
}
