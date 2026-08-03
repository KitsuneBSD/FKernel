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

} // namespace fkernel
