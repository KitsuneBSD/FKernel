#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace MemoryManagement {
class BumpAllocator {
private:
    LibC::uintptr_t bump_ptr;
    LibC::uintptr_t bump_end;

    BumpAllocator() = default;

public:
    static BumpAllocator& instance() noexcept
    {

        static BumpAllocator s_instance;
        return s_instance;
    }

    void initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept;

    void* alloc(LibC::size_t size, LibC::size_t alignment = 8) noexcept;

    LibC::size_t remaining() const noexcept;
};

}

inline void* Balloc(LibC::size_t size, LibC::size_t alignment = 8)
{
    return MemoryManagement::BumpAllocator::instance().alloc(size, alignment);
}
