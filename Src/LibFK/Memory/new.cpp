#include <LibC/stddef.h>              // For size_t
#include <LibFK/Memory/heap_malloc.h> // For heap_malloc and heap_free

// Define global new and delete operators outside of any namespace.
// These operators are typically defined at the global scope.

// Global operator new
void *operator new(size_t size) { return heap_malloc(size); }

// Global operator new[]
void *operator new[](size_t size) { return heap_malloc(size); }

// Global operator delete
void operator delete(void *ptr) noexcept { heap_free(ptr); }

// Global operator delete[]
void operator delete[](void *ptr) noexcept { heap_free(ptr); }

// Global operator delete with size (for C++14 and later, or specific compiler
// extensions) This overload is often provided for completeness, though not
// strictly required by all C++ standards for basic new/delete.
void operator delete(void *ptr, size_t) noexcept { heap_free(ptr); }

// Global operator delete[] with size
void operator delete[](void *ptr, size_t) noexcept { heap_free(ptr); }
