  #pragma once

  #include <LibC/assert.h>
  #include <LibC/stddef.h>
  #include <LibFK/new.h>
  
  template<typename T, size_t MaxBits>
  class Bitmap {
  private:
      size_t m_size;
      T m_bits[(MaxBits + sizeof(T) * 8 - 1) / (sizeof(T) * 8)];

  public:
      Bitmap() : m_size(0) {
          clear();
      }

      explicit Bitmap(size_t size) : m_size(size) {
          clear();
      }

      size_t sizeInBytes() const noexcept {
          return (m_size + 7) / 8;
      }

      const T* data() const noexcept { return m_bits; }
      T* data() noexcept { return m_bits; }

      bool get(size_t index) const noexcept {
          return (m_bits[index / (sizeof(T) * 8)] & (T(1) << (index % (sizeof(T) * 8)))) != 0;
      }

      void set(size_t index, bool value) noexcept {
          if (value) {
              m_bits[index / (sizeof(T) * 8)] |= (T(1) << (index % (sizeof(T) * 8)));
          } else {
              m_bits[index / (sizeof(T) * 8)] &= ~(T(1) << (index % (sizeof(T) * 8)));
          }
      }

      void clear() noexcept {
          memset(m_bits, 0, sizeof(m_bits));
      }

      void resize(size_t size) noexcept {
          m_size = size;
          clear();
      }
  };