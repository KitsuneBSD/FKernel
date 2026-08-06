#include <LibFK/Algorithms/Crypto/sha1.h>
#include <LibFK/Utilities/memory.h>

namespace fk::algorithms {

static uint32_t rotl32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static uint32_t be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void store_be32(uint8_t* p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static void sha1_compress(Sha1State* s, const uint8_t blk[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i)
        w[i] = be32(blk + i * 4);
    for (int i = 16; i < 80; ++i)
        w[i] = rotl32(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

    uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3], e = s->h[4];

    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);       k = 0x5A827999; }
        else if (i < 40) { f = b ^ c ^ d;                  k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
        else             { f = b ^ c ^ d;                  k = 0xCA62C1D6; }
        uint32_t tmp = rotl32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl32(b, 30); b = a; a = tmp;
    }

    s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

void sha1_init(Sha1State* s) {
    s->h[0] = 0x67452301; s->h[1] = 0xEFCDAB89;
    s->h[2] = 0x98BADCFE; s->h[3] = 0x10325476; s->h[4] = 0xC3D2E1F0;
    s->count = 0; s->buf_used = 0;
}

void sha1_update(Sha1State* s, const uint8_t* data, size_t len) {
    s->count += len;
    while (len > 0) {
        size_t space = 64 - s->buf_used;
        size_t take  = len < space ? len : space;
        fk::memory::copy(s->buf + s->buf_used, data, take);
        s->buf_used += take; data += take; len -= take;
        if (s->buf_used == 64) {
            sha1_compress(s, s->buf);
            s->buf_used = 0;
        }
    }
}

void sha1_final(Sha1State* s, uint8_t out[20]) {
    uint64_t bit_count = s->count * 8;
    uint8_t pad = 0x80;
    sha1_update(s, &pad, 1);
    uint8_t z = 0;
    while (s->buf_used != 56)
        sha1_update(s, &z, 1);
    uint8_t len_be[8];
    for (int i = 7; i >= 0; --i) { len_be[i] = bit_count & 0xFF; bit_count >>= 8; }
    sha1_update(s, len_be, 8);
    for (int i = 0; i < 5; ++i) store_be32(out + i * 4, s->h[i]);
}

void sha1(const uint8_t* data, size_t len, uint8_t out[20]) {
    Sha1State s;
    sha1_init(&s);
    sha1_update(&s, data, len);
    sha1_final(&s, out);
}

void hmac_sha1(const uint8_t* key, size_t klen, const uint8_t* msg, size_t mlen, uint8_t out[20]) {
    uint8_t k[64], ik[64], ok[64];
    fk::memory::set(k, 0, 64);
    if (klen > 64) { sha1(key, klen, k); }
    else            { fk::memory::copy(k, key, klen); }
    for (int i = 0; i < 64; ++i) { ik[i] = k[i] ^ 0x36; ok[i] = k[i] ^ 0x5C; }

    Sha1State s;
    sha1_init(&s);
    sha1_update(&s, ik, 64);
    sha1_update(&s, msg, mlen);
    uint8_t inner[20];
    sha1_final(&s, inner);

    sha1_init(&s);
    sha1_update(&s, ok, 64);
    sha1_update(&s, inner, 20);
    sha1_final(&s, out);
}

void pbkdf2_hmac_sha1(const uint8_t* pw, size_t pwlen,
                      const uint8_t* salt, size_t slen,
                      uint32_t iters, uint8_t* out, size_t outlen) {
    uint8_t salt_ext[64 + 4];
    size_t  safe_slen = slen < 60 ? slen : 60;
    fk::memory::copy(salt_ext, salt, safe_slen);

    uint32_t block = 1;
    while (outlen > 0) {
        salt_ext[safe_slen + 0] = (block >> 24) & 0xFF;
        salt_ext[safe_slen + 1] = (block >> 16) & 0xFF;
        salt_ext[safe_slen + 2] = (block >>  8) & 0xFF;
        salt_ext[safe_slen + 3] =  block        & 0xFF;

        uint8_t u[20], t[20];
        hmac_sha1(pw, pwlen, salt_ext, safe_slen + 4, u);
        fk::memory::copy(t, u, 20);

        for (uint32_t i = 1; i < iters; ++i) {
            hmac_sha1(pw, pwlen, u, 20, u);
            for (int j = 0; j < 20; ++j) t[j] ^= u[j];
        }

        size_t take = outlen < 20 ? outlen : 20;
        fk::memory::copy(out, t, take);
        out += take; outlen -= take; ++block;
    }
}

} // namespace fk::algorithms
