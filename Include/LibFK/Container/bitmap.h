#pragma once

#include <LibC/assert.h>
#include <LibC/string.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T, size_t MaxBits> class Bitmap {
private:
  size_t m_size;
  T m_bits[(MaxBits + sizeof(T) * 8 - 1) / (sizeof(T) * 8)];

public:
  Bitmap() : m_size(0) { memset(m_bits, 0, sizeof(m_bits)); }

  explicit Bitmap(size_t size) : m_size(size) {
    memset(m_bits, 0, sizeof(m_bits));
  }

  size_t sizeInBytes() const noexcept { return (m_size + 7) / 8; }

  const T *data() const noexcept { return m_bits; }
  T *data() noexcept { return m_bits; }

  bool get(size_t index) const noexcept {
    return (m_bits[index / (sizeof(T) * 8)] &
            (T(1) << (index % (sizeof(T) * 8)))) != 0;
  }

  void set(size_t index, bool value) noexcept {
    if (value) {
      m_bits[index / (sizeof(T) * 8)] |= (T(1) << (index % (sizeof(T) * 8)));
    } else {
      m_bits[index / (sizeof(T) * 8)] &= ~(T(1) << (index % (sizeof(T) * 8)));
    }
  }

  void clear(int index) { set(index, false); }

  void clear_all() noexcept { memset(m_bits, 0, sizeof(m_bits)); }

  void resize(size_t size) noexcept {
    m_size = size;
    memset(m_bits, 0, sizeof(m_bits));
  }

  T get_mask_from(size_t index) const noexcept {
    if (index >= m_size) {
      return 0;
    }
    T mask = ~T(0) << (index % (sizeof(T) * 8));
    return m_bits[index / (sizeof(T) * 8)] & mask;
  }

  T get_mask() const noexcept { return m_bits[0]; }

  size_t size() const noexcept { return m_size; }

  bool is_empty() const noexcept { return m_size == 0; }
};

} // namespace containers
} // namespace fk
