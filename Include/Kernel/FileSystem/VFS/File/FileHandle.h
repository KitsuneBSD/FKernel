#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

enum class VNodeType : LibC::uint8_t;
struct VNode;

struct FileHandle {
    VNode* vnode;
    LibC::uint64_t offset;
    LibC::uint32_t flags;

    LibC::ssize_t read(void* buf, LibC::size_t size);
    LibC::ssize_t write(void const* buf, LibC::size_t size);
};

}
