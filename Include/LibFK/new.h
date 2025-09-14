#pragma once

#include <LibFK/heap_malloc.h>

void *operator new(size_t, void *ptr) noexcept { return ptr; }

void *operator new[](size_t, void *ptr) noexcept { return ptr; }

void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *ptr) { return kfree(ptr); }

void operator delete[](void *ptr) { return kfree(ptr); }

void operator delete(void *ptr, size_t) { return kfree(ptr); }

void operator delete[](void *ptr, size_t) { return kfree(ptr); }

void operator delete(void *, void *) noexcept {}

void operator delete[](void *, void *) noexcept {}
