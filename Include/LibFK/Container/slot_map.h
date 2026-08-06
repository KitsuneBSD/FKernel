#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

// SlotMap<T>: dense array with O(1) insert/remove/lookup and generation-tagged keys.
//
// Keys contain a generation counter so stale handles are detected — a lookup
// with an old key for a slot that has been recycled returns nullptr rather than
// silently aliasing a new object.  First consumer: fkernel::ipc::CSpace.
//
// Key layout: { index (32-bit), generation (32-bit) }.
// Generation 0 is reserved — Key{~0u, 0} is the canonical invalid key.

template <typename T>
class SlotMap {
public:
    struct Key {
        uint32_t index;
        uint32_t generation;

        bool is_valid() const { return index != ~0u && generation != 0; }
        bool operator==(const Key& o) const { return index == o.index && generation == o.generation; }
        bool operator!=(const Key& o) const { return !(*this == o); }

        static constexpr Key invalid() { return {~0u, 0u}; }
    };

    SlotMap() = default;
    ~SlotMap() = default;

    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;

    SlotMap(SlotMap&&) = default;
    SlotMap& operator=(SlotMap&&) = default;

    // Insert a value, returning its key. Returns OutOfMemory if allocation fails.
    [[nodiscard]] fk::core::Result<Key, fk::core::Error> insert(T value) {
        uint32_t idx;
        if (m_free_head != ~0u) {
            idx = m_free_head;
            m_free_head = m_slots[idx].m_next_free;
        } else {
            idx = static_cast<uint32_t>(m_slots.size());
            Slot s;
            s.m_generation = 0;
            s.m_next_free  = ~0u;
            TRY(m_slots.push_back(fk::types::move(s)));
        }

        Slot& slot = m_slots[idx];
        ++slot.m_generation;
        if (slot.m_generation == 0) ++slot.m_generation; // skip generation 0

        slot.m_value.emplace(fk::types::move(value));
        ++m_active_count;
        return Key{idx, slot.m_generation};
    }

    // Return a pointer to the stored value, or nullptr if the key is stale/invalid.
    T* get(Key key) {
        if (!key.is_valid() || key.index >= m_slots.size()) return nullptr;
        Slot& slot = m_slots[key.index];
        if (slot.m_generation != key.generation || !slot.m_value.has_value()) return nullptr;
        return &slot.m_value.value();
    }

    const T* get(Key key) const {
        if (!key.is_valid() || key.index >= m_slots.size()) return nullptr;
        const Slot& slot = m_slots[key.index];
        if (slot.m_generation != key.generation || !slot.m_value.has_value()) return nullptr;
        return &slot.m_value.value();
    }

    // Remove the value at key. Returns false if the key is stale/invalid.
    bool remove(Key key) {
        if (!key.is_valid() || key.index >= m_slots.size()) return false;
        Slot& slot = m_slots[key.index];
        if (slot.m_generation != key.generation || !slot.m_value.has_value()) return false;

        slot.m_value.reset();
        ++slot.m_generation;
        if (slot.m_generation == 0) ++slot.m_generation; // skip generation 0
        slot.m_next_free = m_free_head;
        m_free_head = key.index;
        --m_active_count;
        return true;
    }

    bool contains(Key key) const { return get(key) != nullptr; }

    size_t size()     const { return m_active_count; }
    bool   is_empty() const { return m_active_count == 0; }

    // Iterate active slots. fn receives (Key, T&).
    template <typename Fn>
    void for_each(Fn&& fn) {
        for (size_t i = 0; i < m_slots.size(); ++i) {
            Slot& slot = m_slots[i];
            if (!slot.m_value.has_value()) continue;
            fn(Key{static_cast<uint32_t>(i), slot.m_generation}, slot.m_value.value());
        }
    }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (size_t i = 0; i < m_slots.size(); ++i) {
            const Slot& slot = m_slots[i];
            if (!slot.m_value.has_value()) continue;
            fn(Key{static_cast<uint32_t>(i), slot.m_generation}, slot.m_value.value());
        }
    }

    void clear() {
        for (size_t i = 0; i < m_slots.size(); ++i)
            m_slots[i].m_value.reset();
        m_slots.clear();
        m_free_head    = ~0u;
        m_active_count = 0;
    }

private:
    struct Slot {
        fk::memory::optional<T> m_value{};
        uint32_t                m_generation{0};
        uint32_t                m_next_free{~0u};
    };

    Vector<Slot> m_slots{};
    uint32_t     m_free_head{~0u};
    size_t       m_active_count{0};
};

} // namespace containers
} // namespace fk
