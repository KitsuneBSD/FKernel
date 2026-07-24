#include <LibC/string.h>

/* No locale support: strcoll is equivalent to strcmp */
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
