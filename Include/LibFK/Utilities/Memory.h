#pragma once

#include <LibC/string.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

inline void* copy(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

inline void* move(void* dest, const void* src, size_t n) {
    return memmove(dest, src, n);
}

inline void* set(void* s, int c, size_t n) {
    return memset(s, c, n);
}

inline int compare(const void* s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n);
}

} // namespace memory
} // namespace fk
