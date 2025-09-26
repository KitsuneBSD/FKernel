#pragma once

#include <LibC/stddef.h>

template <typename T, size_t bits> class Bitmap {
private:
  static constexpr size_t BITS_PER_ELEM = sizeof(T) * 8;
  static constexpr size_t ELEMS = (bits + BITS_PER_ELEM - 1) / BITS_PER_ELEM;

  static constexpr size_t elem_index(size_t idx) noexcept {
    return __builtin_expect(idx >> __builtin_ctz(BITS_PER_ELEM), 0);
  }

  static constexpr size_t bit_index(size_t idx) noexcept {
    return idx & (BITS_PER_ELEM - 1);
  }

private:
  T data[ELEMS] = {};

public:
  constexpr bool get(size_t index) const noexcept {
    return (data[elem_index(index)] >> bit_index(index)) & 1;
  }

  constexpr void set(size_t index, bool value) noexcept {
    size_t e = elem_index(index);
    size_t b = bit_index(index);
    if (value)
      data[e] |= T(1) << b;
    else
      data[e] &= ~(T(1) << b);
  }

  constexpr void clear() noexcept {
    for (size_t i = 0; i < ELEMS; ++i)
      data[i] = T(0);
  }

  constexpr void fill() noexcept {
    for (size_t i = 0; i < ELEMS; ++i)
      data[i] = ~T(0);
  }

  constexpr void populate(size_t counter) {
    for (size_t i = 0; i < counter; ++i)
      set(i, true);
  }

  static constexpr size_t sizeInBytes() noexcept { return ELEMS * sizeof(T); }
};
