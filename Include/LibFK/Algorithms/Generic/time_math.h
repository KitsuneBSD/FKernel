#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Convert a (sec, nsec) duration to timer ticks at the given frequency.
// Mirrors the shared time-to-ticks math used by timerfd and event timers.
inline uint64_t timespec_to_ticks(int64_t sec, int64_t nsec, uint32_t frequency) {
    if (frequency == 0) return 0;
    uint64_t sec_ticks   = static_cast<uint64_t>(sec) * frequency;
    uint64_t nsec_ticks  = static_cast<uint64_t>(nsec) * frequency / 1000000000ULL;
    return sec_ticks + nsec_ticks;
}

} // namespace algorithms
} // namespace fk
