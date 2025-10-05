#include <LibC/string.h>

char *strtok(char *str, const char *delim) {
    char *s;
    if (str) {
        s = str;
    } else {
        s = strtok_saveptr;
        if (!s) return NULL;
    }

    // pula delimitadores iniciais
    while (*s && strchr(delim, *s)) s++;
    if (!*s) return NULL;

    char *token_start = s;

    while (*s && !strchr(delim, *s)) s++;

    if (*s) {
        *s = '\0';
        strtok_saveptr = s + 1;
    } else {
        strtok_saveptr = NULL;
    }

    return token_start;
}