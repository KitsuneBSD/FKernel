#include <LibFK/heap_malloc.h>
#include <LibFK/new.h>


void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *ptr) noexcept { kfree(ptr); }

void operator delete[](void *ptr) noexcept { kfree(ptr); }

void operator delete(void *ptr, size_t) noexcept { kfree(ptr); }

void operator delete[](void *ptr, size_t) noexcept { kfree(ptr); }
