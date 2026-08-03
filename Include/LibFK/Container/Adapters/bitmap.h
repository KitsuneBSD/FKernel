#pragma once

#include <LibC/string.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T>
class Bitmap {
public:
  Bitmap() = default;

  Bitmap(T* storage, size_t capacity_bits) {
    if (!storage || capacity_bits == 0) return;

    m_bits     = storage;
    m_capacity = capacity_bits;
    m_size     = capacity_bits;

    clear_all();
  }

  T* data() const noexcept {
    return m_bits;
  }

  ssize_t alloc() noexcept {
    size_t word_count = (m_size + BITS_PER_WORD - 1) / BITS_PER_WORD;

    // Two-pass: start at hint, wrap around if needed.
    for (size_t pass = 0; pass < 2; ++pass) {
      size_t start = (pass == 0) ? m_alloc_hint : 0;
      size_t end   = (pass == 0) ? word_count   : m_alloc_hint;

      for (size_t wi = start; wi < end; ++wi) {
        T w = m_bits[wi];
        if (~w == 0) continue; // all bits used

        for (size_t bit = 0; bit < BITS_PER_WORD; ++bit) {
          size_t index = wi * BITS_PER_WORD + bit;
          if (index >= m_size) break;
          if (!(w & (T(1) << bit))) {
            m_bits[wi] |= (T(1) << bit);
            m_alloc_hint = wi;
            return static_cast<ssize_t>(index);
          }
        }
      }
    }

    return -1;
  }

  bool get(size_t index) const noexcept {
    return (m_bits[word(index)] & mask(index)) != 0;
  }

  void set(size_t index, bool value) noexcept {
    if (value) {
      m_bits[word(index)] |= mask(index);
    } else {
      m_bits[word(index)] &= ~mask(index);
      // Freed a bit before the hint — next alloc can start earlier.
      if (word(index) < m_alloc_hint) m_alloc_hint = word(index);
    }
  }

  void clear(size_t index) noexcept {
    set(index, false);
  }

  void clear_all() noexcept {
    memset(m_bits, 0, capacity_bytes());
    m_alloc_hint = 0;
  }

  size_t size() const noexcept {
    return m_size;
  }

private:
  static constexpr size_t BITS_PER_WORD = sizeof(T) * 8;

  static constexpr size_t word(size_t bit) {
    return bit / BITS_PER_WORD;
  }

  static constexpr T mask(size_t bit) {
    return T(1) << (bit % BITS_PER_WORD);
  }

  size_t capacity_bytes() const noexcept {
    return ((m_capacity + BITS_PER_WORD - 1) / BITS_PER_WORD) * sizeof(T);
  }

private:
  T*     m_bits{nullptr};
  size_t m_capacity{0};
  size_t m_size{0};
  size_t m_alloc_hint{0};   // word index to start alloc scan from (O(1) amortized)
};

} // namespace containers
} // namespace fk
