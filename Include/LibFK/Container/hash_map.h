#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Functional/Function.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

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

  size_t size() const;

  bool is_empty() const;

private:
  struct Entry {
    Key key;
    Value value;
    bool occupied = false;
  };

  fk::containers::Vector<Entry> m_entries;
  size_t m_size{0};

  size_t hash(const Key &key) const;

  ssize_t find_entry_index(const Key &key) const;
};

} // namespace containers
} // namespace fk

namespace fk {
namespace containers {
template <typename T> struct DefaultHasher {
  size_t operator()(const T &value) const { return static_cast<size_t>(value); }
};

template <> struct DefaultHasher<fk::text::String> {
  size_t operator()(const fk::text::String &value) const {
    size_t hash = 5381;
    for (size_t i = 0; i < value.length(); ++i) {
      hash = ((hash << 5) + hash) + value[i];
    }
    return hash;
  }
};

template <typename Key, typename Value>
HashMap<Key, Value>::HashMap() {
  m_entries.resize(16);
}

template <typename Key, typename Value> HashMap<Key, Value>::~HashMap() {}

template <typename Key, typename Value>
size_t HashMap<Key, Value>::hash(const Key &key) const {
  DefaultHasher<Key> hasher;
  return hasher(key) % m_entries.capacity();
}

template <typename Key, typename Value>
ssize_t HashMap<Key, Value>::find_entry_index(const Key &key) const {
  size_t index = hash(key);
  size_t start_index = index;

  while (m_entries[index].occupied) {
    if (m_entries[index].key == key) {
      return index;
    }
    index = (index + 1) % m_entries.capacity();
    if (index == start_index) {
      break; // Full circle, key not found
    }
  }
  return -1;
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::insert(const Key &key,
                                                   const Value &value) {
  if (m_size >= m_entries.capacity() / 2) {
    return fk::core::Error::OutOfMemory;
  }

  size_t index = hash(key);
  size_t start_index = index;

  while (m_entries[index].occupied) {
    if (m_entries[index].key == key) {
      m_entries[index].value = value;
      return true;
    }
    index = (index + 1) % m_entries.capacity();
    if (index == start_index) {
      return fk::core::Error::OutOfMemory;
    }
  }

  m_entries[index].key = key;
  m_entries[index].value = value;
  m_entries[index].occupied = true;
  m_size++;
  return true;
}

template <typename Key, typename Value>
fk::memory::optional<Value> HashMap<Key, Value>::get(const Key &key) const {
  ssize_t index = find_entry_index(key);
  if (index != -1) {
    return m_entries[index].value;
  }
  return {};
}

template <typename Key, typename Value>
fk::core::Result<bool> HashMap<Key, Value>::remove(const Key &key) {
  ssize_t index = find_entry_index(key);
  if (index != -1) {
    m_entries[index].occupied = false;
    m_size--;
    return true;
  }
  return false;
}

template <typename Key, typename Value>
bool HashMap<Key, Value>::contains(const Key &key) const {
  return find_entry_index(key) != -1;
}

template <typename Key, typename Value>
size_t HashMap<Key, Value>::size() const {
  return m_size;
}

template <typename Key, typename Value>
bool HashMap<Key, Value>::is_empty() const {
  return m_size == 0;
}

} // namespace containers
} // namespace fk
