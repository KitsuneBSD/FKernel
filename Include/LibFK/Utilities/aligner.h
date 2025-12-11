#pragma once 

#include <LibFK/Types/types.h>

namespace fk {
namespace utilities {

template <typename T>
constexpr T align_up(T value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

template <typename T>
constexpr T align_down(T value, size_t alignment) {
    return value & ~(alignment - 1);
}

template <typename T>
constexpr bool is_aligned(T value, size_t alignment) {
    return (value & (alignment - 1)) == 0;
}

}
}