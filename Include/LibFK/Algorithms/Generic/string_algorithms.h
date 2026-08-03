#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Case-insensitive ASCII character comparison
static inline char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
}

// Case-insensitive string equality (null-terminated)
static inline bool iequal(const char* a, const char* b) {
    while (*a && *b) {
        if (to_upper(*a) != to_upper(*b)) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

// Case-insensitive string equality with explicit length
static inline bool iequal_n(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (to_upper(a[i]) != to_upper(b[i])) return false;
    }
    return true;
}

} // namespace algorithms
} // namespace fk
