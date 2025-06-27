#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace MemoryManagement {
class BumpAllocator {
private:
    static LibC::uintptr_t bump_ptr;
    static LibC::uintptr_t bump_end;

public:
    static void initialize(LibC::uintptr_t start, LibC::uintptr_t end) noexcept;

    static void* alloc(LibC::size_t size, LibC::size_t alignment = 8) noexcept;

    static LibC::size_t remaining() noexcept;
};

}
