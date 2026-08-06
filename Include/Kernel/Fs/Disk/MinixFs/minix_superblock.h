#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint16_t MINIX1_MAGIC          = 0x137F;
static constexpr uint16_t MINIX1_MAGIC30        = 0x138F;
static constexpr uint32_t MINIX_BLOCK_SIZE      = 1024;
static constexpr uint32_t MINIX_INODES_PER_BLOCK = 32;
static constexpr uint32_t MINIX_DIRECT_ZONES    = 7;
static constexpr uint32_t MINIX_PTR_PER_BLOCK   = 512;

struct MinixSuperBlock {
    uint16_t s_ninodes;
    uint16_t s_nzones;
    uint16_t s_imap_blocks;
    uint16_t s_zmap_blocks;
    uint16_t s_firstdatazone;
    uint16_t s_log_zone_size;
    uint32_t s_max_size;
    uint16_t s_magic;
    uint16_t s_state;
} __attribute__((packed));

} // namespace fkernel
