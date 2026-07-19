#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Utilities/pair.h>

namespace fk {
namespace containers {

template <typename K, typename V>
class Map {
public:
  using Entry = fk::utilities::Pair<K, V>;
  using iterator = Entry *;
  using const_iterator = const Entry *;

  Map() = default;

  bool insert(const K &key, const V &value) {
    size_t idx = lower_bound(key);
    if (idx < m_entries.size() && m_entries[idx].first == key)
      return false;
    m_entries.insert_at(idx, Entry{key, value});
    return true;
  }

  bool insert_or_assign(const K &key, const V &value) {
    size_t idx = lower_bound(key);
    if (idx < m_entries.size() && m_entries[idx].first == key) {
      m_entries[idx].second = value;
      return false;
    }
    m_entries.insert_at(idx, Entry{key, value});
    return true;
  }

  fk::memory::optional<V> get(const K &key) const {
    size_t idx = lower_bound(key);
    if (idx < m_entries.size() && m_entries[idx].first == key)
      return fk::memory::optional<V>(m_entries[idx].second);
    return fk::memory::optional<V>();
  }

  V &operator[](const K &key) {
    size_t idx = lower_bound(key);
    if (idx >= m_entries.size() || !(m_entries[idx].first == key))
      m_entries.insert_at(idx, Entry{key, V{}});
    return m_entries[idx].second;
  }

  bool contains(const K &key) const {
    size_t idx = lower_bound(key);
    return idx < m_entries.size() && m_entries[idx].first == key;
  }

  bool remove(const K &key) {
    size_t idx = lower_bound(key);
    if (idx >= m_entries.size() || !(m_entries[idx].first == key))
      return false;
    m_entries.remove_at(idx);
    return true;
  }

  size_t size() const { return m_entries.size(); }
  bool is_empty() const { return m_entries.is_empty(); }
  void clear() { m_entries.clear(); }

  iterator begin() { return m_entries.begin(); }
  iterator end() { return m_entries.end(); }
  const_iterator begin() const { return m_entries.begin(); }
  const_iterator end() const { return m_entries.end(); }

private:
  size_t lower_bound(const K &key) const {
    size_t lo = 0;
    size_t hi = m_entries.size();
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      if (m_entries[mid].first < key)
        lo = mid + 1;
      else
        hi = mid;
    }
    return lo;
  }

  Vector<Entry> m_entries;
};

} // namespace containers
} // namespace fk
