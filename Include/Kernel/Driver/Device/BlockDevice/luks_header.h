#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

static constexpr uint32_t LUKS_MAGIC_LO = 0x4C554B53; // "LUKS"
static constexpr uint32_t LUKS_MAGIC_HI = 0xBAAE0000; // "\xba\xbe" + version 1
static constexpr uint32_t LUKS_KEY_SLOTS = 8;
static constexpr uint32_t LUKS_ACTIVE    = 0x00AC71F3;
static constexpr uint32_t LUKS_DISABLED  = 0x0000DEAD;
static constexpr uint32_t LUKS_AF_STRIPES = 4000;

struct __attribute__((packed)) LuksKeySlot {
    uint32_t active;
    uint32_t iterations;
    uint8_t  salt[32];
    uint32_t key_material_offset;
    uint32_t stripes;
};

struct __attribute__((packed)) LuksHeader {
    uint8_t  magic[6];
    uint16_t version;
    char     cipher_name[32];
    char     cipher_mode[32];
    char     hash_spec[32];
    uint32_t payload_offset;
    uint32_t key_bytes;
    uint8_t  mk_digest[20];
    uint8_t  mk_digest_salt[32];
    uint32_t mk_digest_iter;
    char     uuid[40];
    LuksKeySlot key_slots[LUKS_KEY_SLOTS];
};

} // namespace fkernel
