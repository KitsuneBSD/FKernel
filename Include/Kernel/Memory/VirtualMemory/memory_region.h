#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>

namespace fkernel {

struct MemoryRegion {
    uintptr_t start;
    uintptr_t end;
    PageFlags flags;
    const char* name;

    bool contains(uintptr_t addr) const {
        return addr >= start && addr < end;
    }
};

} // namespace fkernel
