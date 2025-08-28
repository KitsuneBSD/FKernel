#pragma once

#include <LibC/stdint.h>

namespace FileSystem {

enum class VNodeType : LibC::uint8_t {
    Unknown,
    File,
    Directory,
    Symlink,
    Device,
    Mountpoint
};
}
