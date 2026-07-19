#pragma once

#include <LibC/assert.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T>
class Deque {
public:
  Deque() = default;

  void push_back(const T &value) { m_data.push_back(value); }
  void push_back(T &&value) { m_data.push_back(fk::types::move(value)); }

  void push_front(const T &value) { m_data.insert_at(0, value); }

  void pop_back() { m_data.pop_back(); }

  void pop_front() {
    if (!is_empty())
      m_data.remove_at(0);
  }

  T &front() {
    ASSERT(!is_empty());
    return m_data[0];
  }

  const T &front() const {
    ASSERT(!is_empty());
    return m_data[0];
  }

  T &back() {
    ASSERT(!is_empty());
    return m_data[m_data.size() - 1];
  }

  const T &back() const {
    ASSERT(!is_empty());
    return m_data[m_data.size() - 1];
  }

  T &operator[](size_t index) { return m_data[index]; }
  const T &operator[](size_t index) const { return m_data[index]; }

  size_t size() const { return m_data.size(); }
  bool is_empty() const { return m_data.is_empty(); }
  void clear() { m_data.clear(); }

  T *begin() { return m_data.begin(); }
  T *end() { return m_data.end(); }
  const T *begin() const { return m_data.begin(); }
  const T *end() const { return m_data.end(); }

private:
  Vector<T> m_data;
};

} // namespace containers
} // namespace fk
