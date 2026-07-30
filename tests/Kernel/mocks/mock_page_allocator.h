#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Minimal host-side page allocator mock for kernel unit tests.
// Wraps malloc/free with a 4 KiB alignment guarantee.

namespace test_mocks {

static constexpr size_t PAGE_SIZE = 4096;

inline void* alloc_page() {
    void* p = nullptr;
    if (posix_memalign(&p, PAGE_SIZE, PAGE_SIZE) != 0) return nullptr;
    memset(p, 0, PAGE_SIZE);
    return p;
}

inline void* alloc_pages(size_t count) {
    void* p = nullptr;
    size_t bytes = count * PAGE_SIZE;
    if (posix_memalign(&p, PAGE_SIZE, bytes) != 0) return nullptr;
    memset(p, 0, bytes);
    return p;
}

inline void free_page(void* p) { free(p); }

} // namespace test_mocks
