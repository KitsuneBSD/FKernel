#pragma once

#include <LibFK/Algorithms/Generic/binary_search.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Memory/optional.h>

namespace fk {
namespace containers {

template <typename T>
class [[deprecated("Use HashMap<T, bool> or UnorderedSet<T> instead — O(n) insert/remove")]] Set {
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
    return fk::algorithms::lower_bound(m_data.begin(), m_data.size(), value);
  }

  Vector<T> m_data;
};

} // namespace containers
} // namespace fk
