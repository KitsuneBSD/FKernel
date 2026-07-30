#pragma once

#include <stdint.h>

// Host-side timer mock for kernel unit tests.
// Ticks are advanced manually via advance_ticks() so tests control time.

namespace test_mocks {

class MockTimer {
public:
    static MockTimer& the() {
        static MockTimer inst;
        return inst;
    }

    void reset() { m_ticks = 0; }
    void advance_ticks(uint64_t n) { m_ticks += n; }
    uint64_t ticks() const { return m_ticks; }

    // Simulate a 1 ms tick period (1000 Hz kernel clock default).
    uint64_t ms_to_ticks(uint64_t ms) const { return ms; }

private:
    uint64_t m_ticks{0};
};

} // namespace test_mocks
