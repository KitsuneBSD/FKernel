#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Utilities/pair.h>

namespace fk {
namespace containers {

template <typename K, typename V>
class [[deprecated("Use HashMap<K, Vector<V>> instead — O(n) insert/remove, O(k·n) remove_all")]] MultiMap {
public:
  using Entry = fk::utilities::Pair<K, V>;
  using iterator = Entry *;
  using const_iterator = const Entry *;

  MultiMap() = default;

  void insert(const K &key, const V &value) {
    size_t idx = upper_bound(key);
    m_entries.insert_at(idx, Entry{key, value});
  }

  bool contains(const K &key) const {
    size_t idx = lower_bound(key);
    return idx < m_entries.size() && m_entries[idx].first == key;
  }

  size_t count(const K &key) const {
    size_t idx = lower_bound(key);
    size_t n = 0;
    while (idx + n < m_entries.size() && m_entries[idx + n].first == key)
      ++n;
    return n;
  }

  bool remove(const K &key) {
    size_t idx = lower_bound(key);
    if (idx >= m_entries.size() || !(m_entries[idx].first == key))
      return false;
    m_entries.remove_at(idx);
    return true;
  }

  size_t remove_all(const K &key) {
    size_t idx = lower_bound(key);
    size_t n = count(key);
    for (size_t i = 0; i < n; ++i)
      m_entries.remove_at(idx);
    return n;
  }

  Vector<V> get_all(const K &key) const {
    Vector<V> results;
    size_t idx = lower_bound(key);
    while (idx < m_entries.size() && m_entries[idx].first == key) {
      results.push_back(m_entries[idx].second);
      ++idx;
    }
    return results;
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
    size_t lo = 0, hi = m_entries.size();
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      if (m_entries[mid].first < key) lo = mid + 1;
      else hi = mid;
    }
    return lo;
  }

  size_t upper_bound(const K &key) const {
    size_t lo = 0, hi = m_entries.size();
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      if (!(key < m_entries[mid].first)) lo = mid + 1;
      else hi = mid;
    }
    return lo;
  }

  Vector<Entry> m_entries;
};

} // namespace containers
} // namespace fk
