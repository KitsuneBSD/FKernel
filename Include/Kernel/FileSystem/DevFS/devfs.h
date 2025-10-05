#pragma once 

#include <Kernel/FileSystem/VirtualFS/vfs_node.h>
#include <LibFK/Container/static_vector.h>

struct DevFS {
    fixed_string<64> name; 
    VFSOps* ops;
    void *device_data;
};

VFSNode* devfs_register(const char* name, FileType device_type, VFSOps* ops, void* dev_data = nullptr);
void devfs_unregister(const char* name);
void devfs_init();