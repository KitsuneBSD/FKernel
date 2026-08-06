#pragma once

#include <LibC/stdint.h>

namespace fk {
namespace algorithms {

// Monotone ID generator for strong-typedef ID types.
// IdType must have: explicit IdType(uint64_t), uint64_t value().
template <typename IdType>
class IdGenerator {
    uint64_t m_next;
    uint64_t m_max;

public:
    explicit IdGenerator(uint64_t start = 1, uint64_t max = ~uint64_t(0))
        : m_next(start), m_max(max) {}

    // Non-atomic; caller holds lock.
    IdType generate() {
        if (m_next >= m_max) return IdType(0);
        return IdType(m_next++);
    }

    // Atomic: safe without external lock (single-threaded wrapping case still needs
    // the overflow guard above, so prefer generate() inside a lock-held region).
    IdType generate_atomic() {
        if (m_next >= m_max) return IdType(0);
        return IdType(__sync_fetch_and_add(&m_next, 1));
    }

    bool is_allocated(IdType id) const {
        return id.value() > 0 && id.value() < m_next;
    }

    void reset(uint64_t start = 1) { m_next = start; }
    uint64_t next_value() const    { return m_next; }
};

} // namespace algorithms
} // namespace fk
