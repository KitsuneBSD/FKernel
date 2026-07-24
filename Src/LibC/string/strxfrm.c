#include <LibC/string.h>

/* No locale support: strxfrm copies up to n bytes (same as strncpy) */
size_t strxfrm(char *dest, const char *src, size_t n) {
    size_t len = strlen(src);
    if (dest && n > 0) {
        size_t copy = len < n - 1 ? len : n - 1;
        memcpy(dest, src, copy);
        dest[copy] = '\0';
    }
    return len;
}
