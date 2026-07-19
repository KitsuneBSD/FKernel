#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Memory/optional.h>

namespace fk {
namespace containers {

template <typename T>
class Set {
public:
  using iterator = T *;
  using const_iterator = const T *;

  Set() = default;

  bool insert(const T &value) {
    size_t idx = lower_bound(value);
    if (idx < m_data.size() && m_data[idx] == value)
      return false;
    m_data.insert_at(idx, value);
    return true;
  }

  bool contains(const T &value) const {
    size_t idx = lower_bound(value);
    return idx < m_data.size() && m_data[idx] == value;
  }

  bool remove(const T &value) {
    size_t idx = lower_bound(value);
    if (idx >= m_data.size() || !(m_data[idx] == value))
      return false;
    m_data.remove_at(idx);
    return true;
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

  Vector<T> m_data;
};

} // namespace containers
} // namespace fk
