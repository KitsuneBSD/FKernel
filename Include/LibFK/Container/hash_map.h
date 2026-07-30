#pragma once

#include <LibFK/Algorithms/djb2.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Functional/function.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>
#include <LibFK/Types/process_id.h>

namespace fk {
namespace containers {

template <typename Key, typename Value> class HashMap {
public:
  HashMap();
  ~HashMap();

  fk::core::Result<bool> insert(const Key &key, const Value &value);
  fk::memory::optional<Value> get(const Key &key) const;
  fk::core::Result<bool> remove(const Key &key);
  bool contains(const Key &key) const;
  size_t size() const { return m_size; }
  bool is_empty() const { return m_size == 0; }

  // Iterate all occupied entries. Fn receives (const Key&, Value&).
  // Safe to call remove() on keys seen during iteration (Robin Hood probe
  // shifting may cause entries to be visited again; callers must tolerate this).
  template <typename Fn>
  void for_each(Fn&& fn) {
    for (size_t i = 0; i < m_buckets.size(); ++i) {
      if (m_buckets[i].is_occupied())
        fn(m_buckets[i].key, m_buckets[i].value);
    }
  }

  template <typename Fn>
  void for_each(Fn&& fn) const {
    for (size_t i = 0; i < m_buckets.size(); ++i) {
      if (m_buckets[i].is_occupied())
        fn(m_buckets[i].key, m_buckets[i].value);
    }
  }

private:
  static constexpr size_t EMPTY_PSL = (size_t)-1;

  struct Entry {
    Key key;
    Value value;
    size_t psl{EMPTY_PSL}; // probe sequence length; EMPTY_PSL = vacant slot

    bool is_occupied() const { return psl != EMPTY_PSL; }
  };

  fk::containers::Vector<Entry> m_buckets;
  size_t m_size{0};

  size_t bucket_for(const Key &key, size_t cap) const;
  fk::core::Result<bool> rehash(size_t new_cap);
  fk::core::Result<bool> maybe_grow();
};

// DefaultHasher -----------------------------------------------------------

template <typename T> struct DefaultHasher {
  size_t operator()(const T &value) const { return static_cast<size_t>(value); }
};

template <> struct DefaultHasher<fk::text::String> {
  size_t operator()(const fk::text::String &value) const {
    return static_cast<size_t>(fk::algorithms::djb2(value.c_str(), value.length()));
  }
};

template <> struct DefaultHasher<fk::ProcessId> {
  size_t operator()(const fk::ProcessId &id) const {
    return static_cast<size_t>(id.value());
  }
};

template <> struct DefaultHasher<uint32_t> {
  size_t operator()(uint32_t v) const { return static_cast<size_t>(v); }
};

// Implementation ----------------------------------------------------------

template <typename Key, typename Value>
HashMap<Key, Value>::HashMap() {
  m_buckets.resize(16);
}

template <typename Key, typename Value>
HashMap<Key, Value>::~HashMap() {}

template <typename Key, typename Value>
size_t HashMap<Key, Value>::bucket_for(const Key &key, size_t cap) const {
  DefaultHasher<Key> hasher;
  return hasher(key) % cap;
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::rehash(size_t new_cap) {
  fk::containers::Vector<Entry> new_buckets;
  new_buckets.resize(new_cap);
  if (new_buckets.size() < new_cap) return fk::core::Error::OutOfMemory;

  for (size_t i = 0; i < m_buckets.size(); i++) {
    if (!m_buckets[i].is_occupied()) continue;

    // Re-insert into new_buckets using Robin Hood
    Entry to_insert;
    to_insert.key = m_buckets[i].key;
    to_insert.value = m_buckets[i].value;
    to_insert.psl = 0;
    size_t pos = bucket_for(to_insert.key, new_cap);

    while (true) {
      if (!new_buckets[pos].is_occupied()) {
        new_buckets[pos] = to_insert;
        break;
      }
      if (new_buckets[pos].psl < to_insert.psl) {
        // Robin Hood: steal from the rich
        Entry displaced = new_buckets[pos];
        new_buckets[pos] = to_insert;
        to_insert = displaced;
      }
      to_insert.psl++;
      pos = (pos + 1) % new_cap;
    }
  }

  m_buckets = static_cast<fk::containers::Vector<Entry>&&>(new_buckets);
  return true;
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::maybe_grow() {
  size_t cap = m_buckets.size();
  // Grow at 75% load factor
  if (m_size * 4 < cap * 3) return true;
  return rehash(cap * 2);
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::insert(const Key &key, const Value &value) {
  auto grow_res = maybe_grow();
  if (grow_res.is_error()) return grow_res.error();

  size_t cap = m_buckets.size();
  Entry to_insert;
  to_insert.key = key;
  to_insert.value = value;
  to_insert.psl = 0;
  size_t pos = bucket_for(key, cap);

  while (true) {
    if (!m_buckets[pos].is_occupied()) {
      m_buckets[pos] = to_insert;
      m_size++;
      return true;
    }
    if (m_buckets[pos].key == to_insert.key) {
      m_buckets[pos].value = to_insert.value; // update
      return true;
    }
    if (m_buckets[pos].psl < to_insert.psl) {
      Entry displaced = m_buckets[pos];
      m_buckets[pos] = to_insert;
      to_insert = displaced;
    }
    to_insert.psl++;
    pos = (pos + 1) % cap;
  }
}

template <typename Key, typename Value>
fk::memory::optional<Value> HashMap<Key, Value>::get(const Key &key) const {
  size_t cap = m_buckets.size();
  if (cap == 0) return {};
  size_t pos = bucket_for(key, cap);
  size_t psl = 0;

  while (m_buckets[pos].is_occupied()) {
    if (m_buckets[pos].psl < psl) return {};   // cannot be further right
    if (m_buckets[pos].key == key)
      return m_buckets[pos].value;
    psl++;
    pos = (pos + 1) % cap;
  }
  return {};
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::remove(const Key &key) {
  size_t cap = m_buckets.size();
  if (cap == 0) return false;
  size_t pos = bucket_for(key, cap);
  size_t psl = 0;

  while (m_buckets[pos].is_occupied()) {
    if (m_buckets[pos].psl < psl) return false;
    if (m_buckets[pos].key == key) {
      // Backshift deletion: pull subsequent entries left
      m_size--;
      size_t cur = pos;
      while (true) {
        size_t next = (cur + 1) % cap;
        if (!m_buckets[next].is_occupied() || m_buckets[next].psl == 0) {
          m_buckets[cur].psl = EMPTY_PSL;
          break;
        }
        m_buckets[cur] = m_buckets[next];
        m_buckets[cur].psl--;
        cur = next;
      }
      return true;
    }
    psl++;
    pos = (pos + 1) % cap;
  }
  return false;
}

template <typename Key, typename Value>
bool HashMap<Key, Value>::contains(const Key &key) const {
  return get(key).has_value();
}

} // namespace containers
} // namespace fk
