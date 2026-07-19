#include <LibC/string.h>

/* Provided by LibFK/heap_malloc.cpp */
extern void *kmalloc(size_t size);

char *strdup(const char *s) {
  if (!s) return NULL;
  size_t len = strlen(s) + 1;
  char *copy = (char *)kmalloc(len);
  if (!copy) return NULL;
  memcpy(copy, s, len);
  return copy;
}

char *strndup(const char *s, size_t n) {
  if (!s) return NULL;
  size_t len = strnlen(s, n);
  char *copy = (char *)kmalloc(len + 1);
  if (!copy) return NULL;
  memcpy(copy, s, len);
  copy[len] = '\0';
  return copy;
}
