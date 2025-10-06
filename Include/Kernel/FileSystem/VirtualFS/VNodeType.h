#pragma once
#include <LibC/stdint.h>

/**
 * @brief Enum representing the type of a vnode
 */

enum class VNodeType : uint8_t
{
    File,
    Directory,
    Symlink,
    Device
};

