#pragma once

#include <LibFK/Container/fixed_string.h>

#include <Kernel/FileSystem/VirtualFS/vfs.h>

struct Device
{
    fixed_string<64> d_name;
    VNodeType d_type;
    void *driver_data;
    VNodeOps *ops;
};