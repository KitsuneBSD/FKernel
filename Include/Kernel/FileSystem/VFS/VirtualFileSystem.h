#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/intrusiveList.h>

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

class VFS {

public:
    FK::IntrusiveList<MountPoint, &MountPoint::list_node> mount_points_;

    VFS();

    int mount(char const* path, VNode* root);
    MountPoint* find_mount_point(char const* path);
    VNode* resolve_path(char const* path, char* relative_path_out, LibC::size_t relative_path_out_size);
    VNode* lookup(VNode* base, char const* path);
};

}
