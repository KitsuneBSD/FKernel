#pragma once

#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <LibC/stddef.h>

namespace LibFK {

template<typename Key, typename Value, typename HashFunc, typename KeyEqual>
class HashMap {
public:
    struct Entry {
        Key key;
        Value value;
        bool occupied = false;
        bool tombstone = false;
    };

    explicit HashMap(HashFunc hasher = HashFunc {}, KeyEqual key_equal = KeyEqual {})
        : _hasher(hasher)
        , _key_equal(key_equal)
        , _capacity(8)
        , _size(0)
    {
        _entries = allocate(_capacity);
    }

    ~HashMap()
    {
        clear();
        deallocate(_entries);
    }

    bool insert(Key const& key, Value const& value)
    {
        if ((_size + 1) * 2 >= _capacity)
            resize(_capacity * 2);

        LibC::size_t index = find_slot(key);
        if (_entries[index].occupied && !_entries[index].tombstone) {
            // Já existe, atualiza valor
            _entries[index].value = value;
            return false;
        }

        _entries[index].key = key;
        _entries[index].value = value;
        _entries[index].occupied = true;
        _entries[index].tombstone = false;
        ++_size;
        return true;
    }

    Value* find(Key const& key)
    {
        LibC::size_t index = probe(key);
        if (index == npos)
            return nullptr;
        return &_entries[index].value;
    }

    bool erase(Key const& key)
    {
        LibC::size_t index = probe(key);
        if (index == npos)
            return false;

        _entries[index].occupied = false;
        _entries[index].tombstone = true;
        --_size;
        return true;
    }

    void clear()
    {
        for (LibC::size_t i = 0; i < _capacity; ++i)
            _entries[i].occupied = false;
        _size = 0;
    }

    LibC::size_t size() const { return _size; }

private:
    static constexpr LibC::size_t npos = static_cast<LibC::size_t>(-1);

    Entry* _entries;
    LibC::size_t _capacity;
    LibC::size_t _size;
    HashFunc _hasher;
    KeyEqual _key_equal;

    Entry* allocate(LibC::size_t capacity)
    {
        void* mem = Falloc(sizeof(Entry) * capacity);
        FK::enforcef(mem != nullptr, "HashMap allocation failed");
        Entry* entries = static_cast<Entry*>(mem);
        for (LibC::size_t i = 0; i < capacity; ++i)
            new (&entries[i]) Entry();
        return entries;
    }

    void deallocate(Entry* entries)
    {
        if (!entries)
            return;
        for (LibC::size_t i = 0; i < _capacity; ++i)
            entries[i].~Entry();
        Ffree(entries);
    }

    LibC::size_t hash_key(Key const& key) const
    {
        return _hasher(key) % _capacity;
    }

    LibC::size_t find_slot(Key const& key)
    {
        LibC::size_t index = hash_key(key);
        while (_entries[index].occupied && !_entries[index].tombstone && !_key_equal(_entries[index].key, key)) {
            index = (index + 1) % _capacity;
        }
        return index;
    }

    LibC::size_t probe(Key const& key)
    {
        LibC::size_t index = hash_key(key);
        LibC::size_t start = index;
        while (_entries[index].occupied || _entries[index].tombstone) {
            if (_entries[index].occupied && !_entries[index].tombstone && _key_equal(_entries[index].key, key))
                return index;
            index = (index + 1) % _capacity;
            if (index == start)
                break;
        }
        return npos;
    }

    void resize(LibC::size_t new_capacity)
    {
        Entry* old_entries = _entries;
        LibC::size_t old_capacity = _capacity;

        _entries = allocate(new_capacity);
        _capacity = new_capacity;
        _size = 0;

        for (LibC::size_t i = 0; i < old_capacity; ++i) {
            if (old_entries[i].occupied && !old_entries[i].tombstone)
                insert(old_entries[i].key, old_entries[i].value);
        }

        deallocate(old_entries);
    }
};
}
