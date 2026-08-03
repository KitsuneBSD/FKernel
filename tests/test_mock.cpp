// C++ stubs for LibFK allocator functions — map to host libc in test mode.
// test_mock.c already provides C-side stubs (kmalloc, heap_malloc, etc.);
// this file provides the C++ namespace equivalents used by make_ref<T> and
// other LibFK helpers that call fk::memory::allocate/reallocate/free.
#include <LibFK/Memory/Allocators/heap_malloc.h>

// Forward-declare the C allocator stubs from test_mock.c to avoid pulling
// in system <stdlib.h> which conflicts with LibC/string.h declarations.
extern "C" {
    void* kmalloc(size_t size);
    void  kfree(void* ptr);
    void* krealloc(void* ptr, size_t size);
}

namespace fk {
namespace memory {

fk::core::Result<uint64_t*, fk::core::Error> allocate(size_t size) {
    void* ptr = kmalloc(size);
    if (!ptr) return fk::core::Error::OutOfMemory;
    return reinterpret_cast<uint64_t*>(ptr);
}

fk::core::Result<uint64_t*, fk::core::Error> reallocate(uint64_t* ptr, size_t size) {
    void* new_ptr = krealloc(static_cast<void*>(ptr), size);
    if (!new_ptr && size > 0) return fk::core::Error::OutOfMemory;
    return reinterpret_cast<uint64_t*>(new_ptr);
}

void free(uint64_t* ptr) {
    kfree(static_cast<void*>(ptr));
}

} // namespace memory
} // namespace fk
