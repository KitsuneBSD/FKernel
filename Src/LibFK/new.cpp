#include <Kernel/MemoryManager/TlsfHeap.h>
#include <LibFK/new.h>

// TODO/FIXME: Allocation wrapper — review ownership rules, null-return behavior,
// alignment requirements, and concurrency considerations. Prefer RAII or factory
// helpers returning smart pointers when ownership needs to be explicit.
void *heap_malloc(size_t size)
{
    return TLSFHeap::the().alloc(size);
}

// TODO/FIXME: Free wrapper — document that caller must not use pointer after free,
// and consider adding debug checks for double-free or invalid pointers in debug builds.
void heap_free(void *ptr)
{
    TLSFHeap::the().free(ptr);
}

// TODO/FIXME: Global operator new — ensure standard-compliant semantics for zero-size
// allocations and alignment. Consider providing nothrow overloads or documenting
// behavior on allocation failure.
void *operator new(size_t size)
{
    return heap_malloc(size);
}

// TODO/FIXME: Array operator new — same concerns as scalar new; ensure consistent
// behavior and document lifetime/ownership expectations.
void *operator new[](size_t size)
{
    return heap_malloc(size);
}

// TODO/FIXME: Global operator delete — verify matching behavior with operator new,
// handle null pointers safely and consider diagnostics (assertions/logging) in debug builds.
void operator delete(void *ptr) noexcept
{
    heap_free(ptr);
}

// TODO/FIXME: Array operator delete — ensure this matches array allocations and that
// element destructors (if any) are correctly handled by callers.
void operator delete[](void *ptr) noexcept
{
    heap_free(ptr);
}

// TODO/FIXME: Sized delete — consider using the size parameter for optimizations when
// available; document expectations for compiler-generated calls.
void operator delete(void *ptr, size_t) noexcept
{
    heap_free(ptr);
}

// TODO/FIXME: Sized array delete — same as sized scalar delete; verify compiler usage
// and document semantics.
void operator delete[](void *ptr, size_t) noexcept
{
    heap_free(ptr);
}
