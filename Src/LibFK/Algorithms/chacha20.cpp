#include <LibFK/Algorithms/chacha20.h>
#include <LibFK/Utilities/memory.h>

namespace fk::algorithms {

// "expand 32-byte k" as little-endian 32-bit words
static constexpr uint32_t CHACHA_CONST[4] = {
  0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
};

#define CHACHA_QR(a, b, c, d)       \
  a += b; d ^= a; d = (d << 16) | (d >> 16); \
  c += d; b ^= c; b = (b << 12) | (b >> 20); \
  a += b; d ^= a; d = (d << 8)  | (d >> 24); \
  c += d; b ^= c; b = (b << 7)  | (b >> 25)

static ChaCha20PRNG::EntropyFn s_entropy_fn = nullptr;

ChaCha20PRNG::ChaCha20PRNG() {
  fk::memory::set(m_state, 0, sizeof(m_state));
  fk::memory::set(m_buffer, 0, sizeof(m_buffer));
}

ChaCha20PRNG &ChaCha20PRNG::the() {
  static ChaCha20PRNG inst;
  return inst;
}

void ChaCha20PRNG::set_entropy_callback(EntropyFn fn) {
  s_entropy_fn = fn;
}

static uint64_t get_entropy() {
  if (s_entropy_fn) return s_entropy_fn();
  return 0;
}

void ChaCha20PRNG::chacha20_block(uint32_t output[16]) {
  uint32_t x[16];
  x[0]  = CHACHA_CONST[0];
  x[1]  = CHACHA_CONST[1];
  x[2]  = CHACHA_CONST[2];
  x[3]  = CHACHA_CONST[3];
  for (int i = 4; i < 16; ++i) x[i] = m_state[i];
  for (int i = 0; i < 10; ++i) {
    CHACHA_QR(x[0], x[4], x[8],  x[12]);
    CHACHA_QR(x[1], x[5], x[9],  x[13]);
    CHACHA_QR(x[2], x[6], x[10], x[14]);
    CHACHA_QR(x[3], x[7], x[11], x[15]);
    CHACHA_QR(x[0], x[5], x[10], x[15]);
    CHACHA_QR(x[1], x[6], x[11], x[12]);
    CHACHA_QR(x[2], x[7], x[8],  x[13]);
    CHACHA_QR(x[3], x[4], x[9],  x[14]);
  }
  output[0]  = x[0]  + CHACHA_CONST[0];
  output[1]  = x[1]  + CHACHA_CONST[1];
  output[2]  = x[2]  + CHACHA_CONST[2];
  output[3]  = x[3]  + CHACHA_CONST[3];
  for (int i = 4; i < 16; ++i) output[i] = x[i] + m_state[i];
}

void ChaCha20PRNG::refill_buffer() {
  uint32_t block[16];
  chacha20_block(block);
  uint8_t *dst = m_buffer;
  for (int i = 0; i < 16; ++i) {
    uint32_t w = block[i];
    *dst++ = (uint8_t)(w);
    *dst++ = (uint8_t)(w >> 8);
    *dst++ = (uint8_t)(w >> 16);
    *dst++ = (uint8_t)(w >> 24);
  }
  m_buffer_pos = 0;
  uint32_t *key = m_state + 4;
  fk::memory::copy(key, block, 32);
  m_state[12]++;
  fk::memory::set(block, 0, sizeof(block));
}

void ChaCha20PRNG::reseed() {
  m_reseed_counter = 0;
  uint64_t e = get_entropy();
  m_state[4] ^= (uint32_t)(e);
  m_state[5] ^= (uint32_t)(e >> 32);
  m_state[13] ^= (uint32_t)(e >> 16);
  m_state[14] ^= (uint32_t)(e >> 48);
  uint32_t mix[16];
  chacha20_block(mix);
  for (int i = 0; i < 8; ++i) m_state[4 + i] ^= mix[i];
  fk::memory::set(mix, 0, sizeof(mix));
  m_state[12] = 0;
  m_buffer_pos = 64;
  m_reseed_counter = 0;
}

void ChaCha20PRNG::initialize() {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  uint64_t t0 = get_entropy();
  uint64_t t1 = get_entropy();
  m_state[4]  = (uint32_t)(t0);
  m_state[5]  = (uint32_t)(t0 >> 32);
  m_state[6]  = (uint32_t)(t1);
  m_state[7]  = (uint32_t)(t1 >> 32);
  m_state[8]  = (uint32_t)(t0 ^ t1);
  m_state[9]  = (uint32_t)((t0 >> 16) ^ (t1 >> 16));
  m_state[10] = (uint32_t)((t0 >> 32) ^ (t1 >> 48));
  m_state[11] = (uint32_t)((t0 >> 48) ^ (t1 >> 32));
  m_state[12] = 0;
  uint64_t t2 = get_entropy();
  m_state[13] = (uint32_t)(t2);
  m_state[14] = (uint32_t)(t2 >> 32);
  m_state[15] = (uint32_t)(t0 ^ t1 ^ t2);
  for (int i = 0; i < 8; ++i) {
    uint32_t discard[16];
    chacha20_block(discard);
    m_state[12]++;
    fk::memory::set(discard, 0, sizeof(discard));
  }
  m_state[12] = 0;
  refill_buffer();
  m_buffer_pos = 64;
}

void ChaCha20PRNG::fill_buffer(uint8_t *out, size_t count) {
  if (!out || count == 0) return;
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  if (++m_reseed_counter >= 65536)
    reseed();
  else
    m_state[4] ^= (uint32_t)get_entropy();
  size_t remaining = count;
  while (remaining > 0) {
    if (m_buffer_pos >= 64) refill_buffer();
    size_t chunk = 64 - m_buffer_pos;
    if (chunk > remaining) chunk = remaining;
    fk::memory::copy(out, m_buffer + m_buffer_pos, chunk);
    out += chunk;
    m_buffer_pos += chunk;
    remaining -= chunk;
  }
}

} // namespace fk::algorithms
