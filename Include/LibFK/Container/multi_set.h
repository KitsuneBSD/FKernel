#pragma once

#include <LibFK/Container/vector.h>

namespace fk {
namespace containers {

template <typename T>
class MultiSet {
public:
  using iterator = T *;
  using const_iterator = const T *;

  MultiSet() = default;

  void insert(const T &value) {
    size_t idx = upper_bound(value);
    m_data.insert_at(idx, value);
  }

  bool contains(const T &value) const {
    size_t idx = lower_bound(value);
    return idx < m_data.size() && m_data[idx] == value;
  }

  size_t count(const T &value) const {
    size_t idx = lower_bound(value);
    size_t n = 0;
    while (idx + n < m_data.size() && m_data[idx + n] == value)
      ++n;
    return n;
  }

  bool remove(const T &value) {
    size_t idx = lower_bound(value);
    if (idx >= m_data.size() || !(m_data[idx] == value))
      return false;
    m_data.remove_at(idx);
    return true;
  }

  size_t remove_all(const T &value) {
    size_t idx = lower_bound(value);
    size_t n = count(value);
    for (size_t i = 0; i < n; ++i)
      m_data.remove_at(idx);
    return n;
  }

  size_t size() const { return m_data.size(); }
  bool is_empty() const { return m_data.is_empty(); }
  void clear() { m_data.clear(); }

  iterator begin() { return m_data.begin(); }
  iterator end() { return m_data.end(); }
  const_iterator begin() const { return m_data.begin(); }
  const_iterator end() const { return m_data.end(); }

private:
  size_t lower_bound(const T &value) const {
    size_t lo = 0;
    size_t hi = m_data.size();
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      if (m_data[mid] < value)
        lo = mid + 1;
      else
        hi = mid;
    }
    return lo;
  }

  size_t upper_bound(const T &value) const {
    size_t lo = 0;
    size_t hi = m_data.size();
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      if (!(value < m_data[mid]))
        lo = mid + 1;
      else
        hi = mid;
    }
    return lo;
  }

  Vector<T> m_data;
};

} // namespace containers
} // namespace fk
