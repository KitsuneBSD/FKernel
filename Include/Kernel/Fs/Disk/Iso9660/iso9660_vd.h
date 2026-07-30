#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint32_t ISO_SECTOR_SIZE  = 2048;
static constexpr uint32_t ISO_PVD_SECTOR   = 16;

// Volume Descriptor type codes
static constexpr uint8_t ISO_VD_PVD   = 1;
static constexpr uint8_t ISO_VD_SVD   = 2;   // Joliet
static constexpr uint8_t ISO_VD_TERM  = 255;

// Directory record flag bits
static constexpr uint8_t ISO_FLAG_DIR   = 0x02;
static constexpr uint8_t ISO_FLAG_MULTI = 0x80;

// Rock Ridge SUSP field signatures (as two chars)
static constexpr uint16_t SUSP_SP = 0x5053; // "SP" LE
static constexpr uint16_t SUSP_NM = 0x4D4E; // "NM" LE
static constexpr uint16_t SUSP_SL = 0x4C53; // "SL" LE
static constexpr uint16_t SUSP_PX = 0x5850; // "PX" LE
static constexpr uint16_t SUSP_CE = 0x4543; // "CE" LE

// Rock Ridge NM flags
static constexpr uint8_t NM_CONTINUE = 0x01;
static constexpr uint8_t NM_CURRENT  = 0x02;
static constexpr uint8_t NM_PARENT   = 0x04;

// Joliet escape sequences in SVD escape_sequences[32..35]
// "%/@" = UCS-2 Level 1, "%/C" = Level 2, "%/E" = Level 3
static constexpr uint8_t JOLIET_ESC0 = 0x25; // '%'
static constexpr uint8_t JOLIET_ESC1 = 0x2F; // '/'

// Helper: read little-endian uint32 from byte buffer
inline uint32_t iso_read_le32(const uint8_t* buf, size_t off) {
    return static_cast<uint32_t>(buf[off])
         | static_cast<uint32_t>(buf[off+1]) << 8
         | static_cast<uint32_t>(buf[off+2]) << 16
         | static_cast<uint32_t>(buf[off+3]) << 24;
}

// Helper: read little-endian uint16 from byte buffer
inline uint16_t iso_read_le16(const uint8_t* buf, size_t off) {
    return static_cast<uint16_t>(buf[off])
         | static_cast<uint16_t>(buf[off+1]) << 8;
}

} // namespace fkernel
