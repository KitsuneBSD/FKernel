#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/intrusiveList.h>

#include <Kernel/FileSystem/VFS/MountPoint.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {
class VFS {
private:
    VFS();

public:
    FK::IntrusiveList<MountPoint, &MountPoint::list_node> mount_points_;

    static VFS& instance()
    {
        static VFS instance;
        return instance;
    }

    int mount(char const* path, VNode* root);
    MountPoint* find_mount_point(char const* path);
    VNode* create_device_node(VNode* parent, char const* name, VNodeType type, void* private_data, VNodeOperations* ops);
    VNode* resolve_path(char const* path, char* relative_path_out, LibC::size_t relative_path_out_size);
    VNode* lookup(VNode* base, char const* path);
};

}
