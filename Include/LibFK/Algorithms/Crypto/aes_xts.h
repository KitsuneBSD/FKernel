#pragma once

#include <LibFK/Algorithms/Crypto/aes.h>
#include <LibFK/Types/types.h>

namespace fk::algorithms {

// AES-256-XTS: two 256-bit AES keys (IEEE 1619).
// key must be 64 bytes: first 32 bytes = K1 (data key), last 32 bytes = K2 (tweak key).
struct AesXtsKey {
    Aes256Key k1;
    Aes256Key k2;
};

void aes_xts_key_init(const uint8_t key[64], AesXtsKey* out);

// Encrypt or decrypt a data unit (must be a multiple of AES_BLOCK_SIZE).
// sector_num: the logical sector number used as XTS tweak.
void aes_xts_encrypt(const AesXtsKey* key, uint64_t sector_num, uint8_t* data, size_t len);
void aes_xts_decrypt(const AesXtsKey* key, uint64_t sector_num, uint8_t* data, size_t len);

} // namespace fk::algorithms
