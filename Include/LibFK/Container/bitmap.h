#pragma once

#include <LibC/string.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T>
class Bitmap {
public:
  Bitmap() = default;

  Bitmap(T* storage, size_t capacity_bits) {
    assert(storage != nullptr);
    assert(capacity_bits > 0);

    m_bits     = storage;
    m_capacity = capacity_bits;
    m_size     = capacity_bits;

    clear_all();
  }

  T* data() const noexcept {
    return m_bits;
  }

  ssize_t alloc() noexcept {
    size_t word_count =
        (m_size + BITS_PER_WORD - 1) / BITS_PER_WORD;

    for (size_t word_idx = 0; word_idx < word_count; ++word_idx) {
      T w = m_bits[word_idx];

      if (~w == 0)
        continue;

      for (size_t bit = 0; bit < BITS_PER_WORD; ++bit) {
        size_t index = word_idx * BITS_PER_WORD + bit;
        if (index >= m_size)
          break;

        if (!(w & (T(1) << bit))) {
          m_bits[word_idx] |= (T(1) << bit);
          return static_cast<ssize_t>(index);
        }
      }
    }

    return -1;
  }

  bool get(size_t index) const noexcept {
    return (m_bits[word(index)] & mask(index)) != 0;
  }

  void set(size_t index, bool value) noexcept {
    if (value)
      m_bits[word(index)] |= mask(index);
    else
      m_bits[word(index)] &= ~mask(index);
  }

  void clear(size_t index) noexcept {
    set(index, false);
  }

  void clear_all() noexcept {
    memset(m_bits, 0, capacity_bytes());
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
};

} // namespace containers
} // namespace fk
