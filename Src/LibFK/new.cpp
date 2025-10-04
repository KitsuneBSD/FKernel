#include <Kernel/MemoryManager/TlsfHeap.h>
#include <LibFK/new.h>

void* heap_malloc(size_t size) {
    return TLSFHeap::the().alloc(size);
}

void heap_free(void* ptr) {
    TLSFHeap::the().free(ptr);
}

void *operator new(size_t size) {
    return heap_malloc(size);
}

void *operator new[](size_t size) {
    return heap_malloc(size);
}

void operator delete(void *ptr) noexcept {
    heap_free(ptr);
}

void operator delete[](void *ptr) noexcept {
    heap_free(ptr);
}

void operator delete(void *ptr, size_t) noexcept {
    heap_free(ptr);
}

void operator delete[](void *ptr, size_t) noexcept {
    heap_free(ptr);
}
