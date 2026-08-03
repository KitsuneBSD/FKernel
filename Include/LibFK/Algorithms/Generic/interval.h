#pragma once

namespace fk {
namespace algorithms {

// True when two inclusive ranges [a_lo, a_hi] and [b_lo, b_hi] overlap.
template <typename T>
inline bool intervals_overlap(T a_lo, T a_hi, T b_lo, T b_hi) {
    return a_lo <= b_hi && b_lo <= a_hi;
}

} // namespace algorithms
} // namespace fk
