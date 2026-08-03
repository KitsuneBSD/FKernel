#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Returns the index of the first element in [data, data+size) that is not
// less than `value` (i.e., the insertion point to keep the range sorted).
// Comparator comp(a, b) returns true if a < b.
template <typename T, typename Compare>
size_t lower_bound(const T* data, size_t size, const T& value, Compare comp) {
    size_t lo = 0, hi = size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (comp(data[mid], value))
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

// Default comparator version using operator<
template <typename T>
size_t lower_bound(const T* data, size_t size, const T& value) {
    return lower_bound(data, size, value, [](const T& a, const T& b) { return a < b; });
}

// Returns the index of the first element strictly greater than `value`.
template <typename T, typename Compare>
size_t upper_bound(const T* data, size_t size, const T& value, Compare comp) {
    size_t lo = 0, hi = size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (!comp(value, data[mid]))
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

template <typename T>
size_t upper_bound(const T* data, size_t size, const T& value) {
    return upper_bound(data, size, value, [](const T& a, const T& b) { return a < b; });
}

} // namespace algorithms
} // namespace fk
