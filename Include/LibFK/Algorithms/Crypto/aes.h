#pragma once

#include <LibFK/Types/types.h>

namespace fk::algorithms {

static constexpr size_t AES_BLOCK_SIZE = 16;
static constexpr size_t AES256_KEY_SIZE = 32;
static constexpr size_t AES256_ROUNDS   = 14;

struct Aes256Key {
    uint32_t rk[4 * (AES256_ROUNDS + 1)];
};

void aes256_key_expand(const uint8_t key[AES256_KEY_SIZE], Aes256Key* out);
void aes256_encrypt(const Aes256Key* key, const uint8_t in[AES_BLOCK_SIZE], uint8_t out[AES_BLOCK_SIZE]);
void aes256_decrypt(const Aes256Key* key, const uint8_t in[AES_BLOCK_SIZE], uint8_t out[AES_BLOCK_SIZE]);

} // namespace fk::algorithms
