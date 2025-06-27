#include <Kernel/MemoryManagement/BumpAllocator/bump_alloc.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

namespace MemoryManagement {
LibC::uintptr_t BumpAllocator::bump_ptr = 0;
LibC::uintptr_t BumpAllocator::bump_end = 0;

void BumpAllocator::initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept
{

    bump_ptr = start;
    bump_end = end;
}
void* BumpAllocator::alloc(LibC::size_t size, LibC::size_t alignment) noexcept
{
    if ((alignment & (alignment - 1)) != 0) {
        Log(LogLevel::ERROR, "BumpAllocator: Alignment must be power of two");
        return nullptr;
    }

    LibC::uintptr_t aligned = (bump_ptr + alignment - 1) & ~(alignment - 1);

    Logf(LogLevel::TRACE, "BumpAllocator: Alloc requested size=%zu alignment=%zu, bump_ptr=0x%p, aligned=0x%p",
        size, alignment, reinterpret_cast<void*>(bump_ptr), reinterpret_cast<void*>(aligned));

    if (aligned + size > bump_end) {
        Log(LogLevel::WARN, "BumpAllocator: Out of memory");
        return nullptr;
    }

    void* ptr = reinterpret_cast<void*>(aligned);
    bump_ptr = aligned + size;

    Logf(LogLevel::TRACE, "BumpAllocator: Allocated ptr=0x%p new bump_ptr=0x%p", ptr, reinterpret_cast<void*>(bump_ptr));

    return ptr;
}

LibC::size_t BumpAllocator::remaining() noexcept
{
    return (bump_end > bump_ptr) ? (bump_end - bump_ptr) : 0;
}
}
