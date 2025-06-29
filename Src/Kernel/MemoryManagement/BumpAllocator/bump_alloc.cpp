#include <Kernel/MemoryManagement/BumpAllocator/bump_alloc.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

namespace MemoryManagement {

void BumpAllocator::initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept
{
    if (initialized) {
        return;
    }

    if (end <= start) {
        Log(LogLevel::ERROR, "BumpAllocator: Invalid heap range (end <= start)");
        return;
    }

    Logf(LogLevel::INFO, "Heap start = %zu, end = %zu, size = %zu",
        start, end,
        (end - start));

    bump_ptr = start;
    bump_end = end;

    Logf(LogLevel::INFO, "BumpAllocator initialized: start=0x%p end=0x%p",
        reinterpret_cast<void*>(start), reinterpret_cast<void*>(end));

    initialized = true;
}

bool BumpAllocator::can_alloc(LibC::size_t size, LibC::size_t alignment) const noexcept
{
    if (!initialized) {
        return false;
    }

    if ((alignment & (alignment - 1)) != 0)
        return false;

    LibC::uintptr_t aligned = (bump_ptr + alignment - 1) & ~(alignment - 1);
    return aligned + size <= bump_end;
}

void* BumpAllocator::alloc_zeroed(LibC::size_t size, LibC::size_t alignment) noexcept
{
    if (!can_alloc(size, alignment)) {
        Logf(LogLevel::ERROR,
            "BumpAllocator: Not enough space for allocation (size=%zu, alignment=%zu, remaining=%zu)",
            size, alignment, remaining());
        return nullptr;
    }

    void* ptr = this->alloc(size, alignment);
    if (ptr)
        LibC::memset(ptr, 0, size);
    return ptr;
}

void* BumpAllocator::alloc(LibC::size_t size, LibC::size_t alignment) noexcept
{
    if ((alignment & (alignment - 1)) != 0) {
        Log(LogLevel::ERROR, "BumpAllocator: Alignment must be power of two");
        return nullptr;
    }

    if (!can_alloc(size, alignment)) {
        Logf(LogLevel::ERROR,
            "BumpAllocator: Not enough space for allocation (size=%zu, alignment=%zu, remaining=%zu)",
            size, alignment, remaining());
        return nullptr;
    }

    LibC::uintptr_t aligned = (bump_ptr + alignment - 1) & ~(alignment - 1);

    void* ptr = reinterpret_cast<void*>(aligned);
    bump_ptr = aligned + size;

    Logf(LogLevel::TRACE,
        "BumpAllocator: Allocated %zu bytes at 0x%p (alignment=%zu), new bump_ptr=0x%p",
        size, ptr, alignment, reinterpret_cast<void*>(bump_ptr));

    return ptr;
}

LibC::size_t BumpAllocator::remaining() const noexcept
{
    return (bump_end > bump_ptr) ? (bump_end - bump_ptr) : 0;
}

}
