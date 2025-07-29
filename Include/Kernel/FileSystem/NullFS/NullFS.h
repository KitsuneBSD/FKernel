#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

VNode* nullfs_create_root();

constexpr char const* NULLFS_DEVICE_NAME = "null";

}
