#include <LibFK/Algorithms/Crypto/aes_xts.h>
#include <LibFK/Utilities/memory.h>

namespace fk::algorithms {

void aes_xts_key_init(const uint8_t key[64], AesXtsKey* out) {
    aes256_key_expand(key,      &out->k1);
    aes256_key_expand(key + 32, &out->k2);
}

// Multiply a 128-bit value in GF(2^128) by x (polynomial: x^128 + x^7 + x^2 + x + 1).
static void gf128_mul_x(uint8_t t[16]) {
    uint8_t carry = 0;
    for (int i = 0; i < 16; ++i) {
        uint8_t next = t[i] >> 7;
        t[i] = (uint8_t)((t[i] << 1) | carry);
        carry = next;
    }
    if (carry) t[0] ^= 0x87;
}

static void xts_crypt(const AesXtsKey* key, uint64_t sector_num,
                      uint8_t* data, size_t len, bool encrypt) {
    uint8_t tweak[16] = {};
    // Tweak = AES_K2(sector_number in little-endian)
    for (int i = 0; i < 8; ++i)
        tweak[i] = (uint8_t)(sector_num >> (i * 8));
    aes256_encrypt(&key->k2, tweak, tweak);

    size_t blocks = len / AES_BLOCK_SIZE;
    for (size_t i = 0; i < blocks; ++i) {
        uint8_t tmp[AES_BLOCK_SIZE];
        uint8_t* blk = data + i * AES_BLOCK_SIZE;

        for (int j = 0; j < 16; ++j) tmp[j] = blk[j] ^ tweak[j];
        if (encrypt) aes256_encrypt(&key->k1, tmp, tmp);
        else         aes256_decrypt(&key->k1, tmp, tmp);
        for (int j = 0; j < 16; ++j) blk[j] = tmp[j] ^ tweak[j];

        gf128_mul_x(tweak);
    }
}

void aes_xts_encrypt(const AesXtsKey* key, uint64_t sector_num, uint8_t* data, size_t len) {
    xts_crypt(key, sector_num, data, len, true);
}

void aes_xts_decrypt(const AesXtsKey* key, uint64_t sector_num, uint8_t* data, size_t len) {
    xts_crypt(key, sector_num, data, len, false);
}

} // namespace fk::algorithms
