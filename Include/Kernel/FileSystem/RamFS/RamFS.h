#pragma once

#include <Kernel/FileSystem/RamFS/RamFSOperations.h>
#include <Kernel/FileSystem/RamFS/RamFSTypes.h>
#include <Kernel/FileSystem/RamFS/RamFSUtils.h>
#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

VNode* ramfs_create_root();
VNode* ramfs_create_unix_tree();

}
