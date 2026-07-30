#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// ext3-specific compatible feature
static constexpr uint32_t EXT3_FEATURE_COMPAT_HAS_JOURNAL = 0x0004;
// Journal inode number (1-based)
static constexpr uint32_t EXT3_JOURNAL_INO = 8;

// JBD magic number (big-endian: 0xC03B3998)
static constexpr uint32_t JBD_MAGIC_NUMBER = 0xC03B3998u;

// JBD block types
static constexpr uint32_t JBD_DESCRIPTOR_BLOCK = 1;
static constexpr uint32_t JBD_COMMIT_BLOCK      = 2;
static constexpr uint32_t JBD_SUPERBLOCK_V1     = 3;
static constexpr uint32_t JBD_SUPERBLOCK_V2     = 4;
static constexpr uint32_t JBD_REVOKE_BLOCK      = 5;

// JBD block tag flags
static constexpr uint16_t JBD_FLAG_ESCAPE     = 0x01; // data started with JBD magic (masked to 0)
static constexpr uint16_t JBD_FLAG_SAME_UUID  = 0x02; // no UUID field follows tag
static constexpr uint16_t JBD_FLAG_DELETED    = 0x04;
static constexpr uint16_t JBD_FLAG_LAST_TAG   = 0x08; // last tag in descriptor

// JBD block header (BIG ENDIAN — all fields must be byte-swapped on x86)
struct JbdHeader {
    uint32_t h_magic;     // JBD_MAGIC_NUMBER (0xC03B3998)
    uint32_t h_blocktype; // JBD_DESCRIPTOR_BLOCK, JBD_COMMIT_BLOCK, etc.
    uint32_t h_sequence;  // transaction sequence number
} __attribute__((packed));

// JBD2 superblock (BIG ENDIAN) — stored at block 0 of the journal file
struct JbdSuperblock {
    JbdHeader s_header;  // h_blocktype = JBD_SUPERBLOCK_V1 or V2

    uint32_t s_blocksize;     // journal block size
    uint32_t s_maxlen;        // total number of journal blocks
    uint32_t s_first;         // first usable journal block (usually 1)
    uint32_t s_sequence;      // sequence of first transaction to replay
    uint32_t s_start;         // journal block where first transaction starts (0 = clean)
    int32_t  s_errno;

    // V2 extension fields (can be zero for V1)
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    uint32_t s_nr_users;
    uint32_t s_dynsuper;
    uint32_t s_max_transaction;
    uint32_t s_max_trans_data;
} __attribute__((packed));

// JBD block tag (BIG ENDIAN) — 8 bytes for 32-bit block numbers
struct JbdBlockTag {
    uint32_t t_blocknr;  // target FS block number (BE)
    uint16_t t_checksum; // entry checksum (BE)
    uint16_t t_flags;    // JBD_FLAG_* (BE)
} __attribute__((packed));

// Byte-swap helpers (journal uses big-endian)
inline uint32_t jbd_be32(uint32_t x) {
    return ((x & 0xFFu) << 24) | ((x >> 8 & 0xFFu) << 16)
         | ((x >> 16 & 0xFFu) << 8) | (x >> 24);
}

inline uint16_t jbd_be16(uint16_t x) {
    return static_cast<uint16_t>(((x & 0xFFu) << 8) | (x >> 8));
}

} // namespace fkernel
