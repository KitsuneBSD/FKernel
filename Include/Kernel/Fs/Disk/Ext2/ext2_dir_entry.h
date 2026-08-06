#pragma once
#include <LibFK/Types/types.h>
namespace fkernel {
struct Ext2DirEntry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    // char name[name_len] follows
} __attribute__((packed));
} // namespace fkernel
