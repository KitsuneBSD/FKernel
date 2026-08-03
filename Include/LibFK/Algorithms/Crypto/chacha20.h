#pragma once

#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fk::algorithms {

/**
 * @brief ChaCha20-based cryptographically secure PRNG (RFC 8439).
 *
 * Fast key erasure provides forward secrecy.  The kernel registers an
 * architecture-specific entropy callback via set_entropy_callback() before
 * calling initialize().  This keeps LibFK free of inline assembly.
 *
 * Used by getrandom(2), /dev/urandom, AT_RANDOM, and ASLR.
 */
class ChaCha20PRNG {
  uint32_t m_state[16];
  uint8_t  m_buffer[64];
  size_t   m_buffer_pos{64};
  uint64_t m_reseed_counter{0};
  fk::synchronization::Spinlock m_lock;

  ChaCha20PRNG();
  ChaCha20PRNG(const ChaCha20PRNG &) = delete;
  ChaCha20PRNG &operator=(const ChaCha20PRNG &) = delete;

  void chacha20_block(uint32_t output[16]);
  void refill_buffer();
  void reseed();

public:
  using EntropyFn = uint64_t (*)();

  static ChaCha20PRNG &the();

  /** Register entropy source. Must be called before initialize(). */
  static void set_entropy_callback(EntropyFn fn);

  void initialize();
  void fill_buffer(uint8_t *out, size_t count);
};

} // namespace fk::algorithms
