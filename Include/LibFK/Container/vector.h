#pragma once

#include <LibC/assert.h>
#include <LibC/string.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T> class Vector {
  struct Metadata {
    size_t size = 0;
    size_t capacity = 0;
  };

  T *m_data = nullptr;
  Metadata m_metadata;

public:
  Vector() = default;
  ~Vector() { clear_and_free(); }

  Vector(const Vector &) = delete;
  Vector &operator=(const Vector &) = delete;

  Vector(Vector &&other) noexcept
      : m_data(other.m_data), m_metadata(other.m_metadata) {
    other.m_data = nullptr;
    other.m_metadata = {0, 0};
  }

  Vector &operator=(Vector &&other) noexcept {
    if (this == &other)
      return *this;

    clear_and_free();
    m_data = other.m_data;
    m_metadata = other.m_metadata;
    other.m_data = nullptr;
    other.m_metadata = {0, 0};
    return *this;
  }

  void push_back(T &&value) {
    ensure_capacity(m_metadata.size + 1);
    new (&m_data[m_metadata.size++]) T(fk::types::move(value));
  }

  void push_back(const T &value) {
    ensure_capacity(m_metadata.size + 1);
    new (&m_data[m_metadata.size++]) T(value);
  }

  void pop_back() {
    if (m_metadata.size == 0)
      return;
    m_data[--m_metadata.size].~T();
  }

  T &operator[](size_t index) {
    ASSERT(index < m_metadata.size);
    return m_data[index];
  }

  const T &operator[](size_t index) const {
    ASSERT(index < m_metadata.size);
    return m_data[index];
  }

  size_t size() const { return m_metadata.size; }
  size_t capacity() const { return m_metadata.capacity; }
  bool is_empty() const { return m_metadata.size == 0; }

  void resize(size_t new_size) {
    ensure_capacity(new_size);
    if (new_size < m_metadata.size) {
        for (size_t i = new_size; i < m_metadata.size; ++i) {
            m_data[i].~T();
        }
    } else {
        for (size_t i = m_metadata.size; i < new_size; ++i) {
            new (&m_data[i]) T();
        }
    }
    m_metadata.size = new_size;
  }

  void clear() {
    for (size_t i = 0; i < m_metadata.size; ++i) {
      m_data[i].~T();
    }
    m_metadata.size = 0;
  }

  T *begin() { return m_data; }
  T *end() { return m_data + m_metadata.size; }
  const T *begin() const { return m_data; }
  const T *end() const { return m_data + m_metadata.size; }

private:
  void ensure_capacity(size_t required) {
    if (required <= m_metadata.capacity)
      return;

    grow(required);
  }

  void grow(size_t required) {
    size_t new_capacity = m_metadata.capacity == 0 ? 4 : m_metadata.capacity * 2;
    if (new_capacity < required)
      new_capacity = required;

    reallocate_to(new_capacity);
  }

  void reallocate_to(size_t new_capacity) {
    T *new_data = static_cast<T *>(kmalloc(new_capacity * sizeof(T)));
    ASSERT(new_data);

    move_elements(new_data);
    free_old();

    m_data = new_data;
    m_metadata.capacity = new_capacity;
  }

  void move_elements(T *new_data) {
    for (size_t i = 0; i < m_metadata.size; ++i) {
      new (&new_data[i]) T(fk::types::move(m_data[i]));
      m_data[i].~T();
    }
  }

  void free_old() {
    if (!m_data)
      return;
    kfree(m_data);
  }

  void clear_and_free() {
    for (size_t i = 0; i < m_metadata.size; ++i) {
      m_data[i].~T();
    }
    free_old();
    m_metadata = {0, 0};
  }
};

} // namespace containers
} // namespace fk
