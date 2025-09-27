#pragma once

#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibFK/new.h>

template<typename T> 
class Bitmap {
  private:
    size_t m_size;
    T* m_bits;
  public:
    Bitmap() : m_size(0), m_bits(nullptr) {}
    Bitmap(size_t size) : m_size(size) {
        m_bits = new T[(size + sizeof(T) * 8 - 1)
                        / (sizeof(T) * 8)]();
    }

    ~Bitmap() {
        delete[] m_bits;
    }

    size_t sizeInBytes() const {
        return (m_size + 7) / 8;
    }

    const T* data() const {
        return m_bits;
    }

    T* data() {
      return m_bits;
    }

    bool get(size_t index) const {
      return m_bits[index / (sizeof(T) * 8)] & (T(1) << (index % (sizeof(T) * 8)));
    }

    bool set(size_t index, bool value) {
      if (value) {
        m_bits[index / (sizeof(T) * 8)] |= (T(1) << (index % (sizeof(T) * 8)));
      } else {
        m_bits[index / (sizeof(T) * 8)] &= ~(T(1) << (index % (sizeof(T) * 8)));
      }
      return value;
    }

    void clear() {
      size_t num_elements = (m_size + sizeof(T) * 8 - 1) / (sizeof(T) * 8);
      for (size_t i = 0; i < num_elements; ++i) {
        m_bits[i] = 0;
      }
    }
};