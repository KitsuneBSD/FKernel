#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stdint.h>

namespace FileSystem {
struct FileStat {
    LibC::uint64_t size;
    LibC::uint32_t permissions;
    LibC::uint64_t inode;

    LibC::uint32_t uid;
    LibC::uint32_t gid;

    VNodeType type;
};

}
