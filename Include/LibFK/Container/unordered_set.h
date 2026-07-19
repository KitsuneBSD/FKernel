#pragma once

#include <LibFK/Container/hash_map.h>

namespace fk {
namespace containers {

template <typename T>
class UnorderedSet {
public:
  UnorderedSet() = default;

  bool insert(const T &value) {
    if (m_map.contains(value))
      return false;
    m_map.insert(value, true);
    return true;
  }

  bool contains(const T &value) const { return m_map.contains(value); }

  bool remove(const T &value) {
    if (!m_map.contains(value))
      return false;
    m_map.remove(value);
    return true;
  }

  size_t size() const { return m_map.size(); }
  bool is_empty() const { return m_map.is_empty(); }

private:
  HashMap<T, bool> m_map;
};

} // namespace containers
} // namespace fk
