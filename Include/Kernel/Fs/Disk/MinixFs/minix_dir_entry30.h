#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct MinixDirEntry30 {
    uint16_t inode;
    char     name[30];
} __attribute__((packed));

} // namespace fkernel
