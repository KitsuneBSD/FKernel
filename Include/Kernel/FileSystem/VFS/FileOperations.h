#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

enum class VNodeType : LibC::uint8_t;
struct VNode;

struct FileStat {
    LibC::uint64_t size;
    LibC::uint32_t permissions;
    LibC::uint64_t inode;

    LibC::uint32_t uid;
    LibC::uint32_t gid;

    VNodeType type;
};

struct FileHandle {
    VNode* vnode;
    LibC::uint64_t offset;
    LibC::uint32_t flags;

    LibC::ssize_t filehandle_read(void* buf, LibC::size_t size);
    LibC::ssize_t filehandle_write(void const* buf, LibC::size_t size);
};

}
