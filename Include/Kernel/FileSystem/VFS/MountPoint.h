#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

struct MountPoint {
    char path[MAX_PATH_LEN];
    VNode* root_vnode;
    FK::IntrusiveNode<MountPoint> list_node;
};

}
