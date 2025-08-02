#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
namespace FileSystem {

struct VNode;

VNode* devfs_init();
bool devfs_register_device(char const* name, VNode* vnode);
int devfs_create_node(VNode* vnode, char const* name, VNodeType type);
}
