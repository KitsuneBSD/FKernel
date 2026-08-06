#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct MinixDirEntry {
    uint16_t inode;
    char     name[14];
} __attribute__((packed));

} // namespace fkernel
