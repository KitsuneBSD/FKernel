#pragma once

#include <LibFK/Types/types.h>

namespace fk::algorithms {

struct Sha1State {
    uint32_t h[5];
    uint64_t count;
    uint8_t  buf[64];
    size_t   buf_used;
};

void sha1_init(Sha1State* s);
void sha1_update(Sha1State* s, const uint8_t* data, size_t len);
void sha1_final(Sha1State* s, uint8_t out[20]);
void sha1(const uint8_t* data, size_t len, uint8_t out[20]);

void hmac_sha1(const uint8_t* key, size_t klen, const uint8_t* msg, size_t mlen, uint8_t out[20]);

void pbkdf2_hmac_sha1(const uint8_t* pw, size_t pwlen,
                      const uint8_t* salt, size_t slen,
                      uint32_t iters,
                      uint8_t* out, size_t outlen);

} // namespace fk::algorithms
