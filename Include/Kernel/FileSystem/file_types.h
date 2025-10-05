#pragma once 

#include <LibC/stdint.h>
#include <LibC/stddef.h>

constexpr size_t VFS_MAX_PATH = (2 << 16);

enum class FileMode : uint32_t
{
    Read = 0x1,
    Write = 0x2,
    ReadWrite = Read | Write, 
    Append = 0x4, 
    Create = 0x8,
    Truncate = 0x10
};

enum class FileType : uint8_t
{
    Unknown,
    Regular,
    Directory,
    CharDevice,
    BlockDevice,
    Symlink
};

struct FilePermissions {
    uint16_t user;
    uint16_t group;
    uint16_t others;
};