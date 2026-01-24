#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace utilities {
template <typename T>
constexpr T min(T a, T b) {
    return (a < b) ? a : b;
}

template <typename T>
constexpr T max(T a, T b) {
    return (a > b) ? a : b;
}

template <typename T>
constexpr T clamp(T value, T min_value, T max_value) {
    if (value < min_value) {
        return min_value;
    } else if (value > max_value) {
        return max_value;
    } else {
        return value;   
    }
}

}
}  // namespace fk
