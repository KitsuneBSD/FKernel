#pragma once

#include <LibC/string.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

// Memory operations

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

// String operations

inline size_t length(const char* s) {
    return strlen(s);
}

inline int compare(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

inline int compare_n(const char* s1, const char* s2, size_t n) {
    return strncmp(s1, s2, n);
}

inline char* copy_string(char* dest, const char* src) {
    return strcpy(dest, src);
}

inline char* copy_n(char* dest, const char* src, size_t n) {
    return strncpy(dest, src, n);
}

inline char* concatenate(char* dest, const char* src) {
    return strcat(dest, src);
}

inline char* concatenate_n(char* dest, const char* src, size_t n) {
    return strncat(dest, src, n);
}

} // namespace memory
} // namespace fk
