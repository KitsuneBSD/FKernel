#include <LibC/stddef.h>
#include <LibC/stdlib.h>

#include <Kernel/MemoryManager/TlsfHeap.h>

void *malloc(size_t size) { return TLSFHeap::the().alloc(size); }

void *calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void *ptr = TLSFHeap::the().alloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void *realloc(void *ptr, size_t size) {
  return TLSFHeap::the().realloc(ptr, size);
}

void free(void *ptr) { TLSFHeap::the().free(ptr); }
