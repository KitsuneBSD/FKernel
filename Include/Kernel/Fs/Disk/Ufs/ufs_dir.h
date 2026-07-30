#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// UFS on-disk directory entry.
// Variable length: fixed header followed by d_namlen bytes of name (not NUL-terminated on disk).
// Entries are 4-byte aligned; d_reclen spans to the start of the next entry.
struct UfsDirEntry {
    uint32_t d_ino;     // inode number (0 = empty slot)
    uint16_t d_reclen;  // bytes to next entry
    uint8_t  d_type;    // file type (DT_* constants)
    uint8_t  d_namlen;  // name length in bytes
    // char d_name[d_namlen] follows immediately
} __attribute__((packed));

// d_type values (matches BSD/Linux DT_* constants)
static constexpr uint8_t UFS_DT_UNKNOWN = 0;
static constexpr uint8_t UFS_DT_FIFO   = 1;
static constexpr uint8_t UFS_DT_CHR    = 2;
static constexpr uint8_t UFS_DT_DIR    = 4;
static constexpr uint8_t UFS_DT_BLK    = 6;
static constexpr uint8_t UFS_DT_REG    = 8;
static constexpr uint8_t UFS_DT_LNK    = 10;
static constexpr uint8_t UFS_DT_SOCK   = 12;
static constexpr uint8_t UFS_DT_WHT    = 14;

} // namespace fkernel
