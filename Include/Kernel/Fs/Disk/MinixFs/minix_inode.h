#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct MinixInode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_mtime;
    uint8_t  i_gid;
    uint8_t  i_nlinks;
    uint16_t i_zone[9];
} __attribute__((packed));

} // namespace fkernel
