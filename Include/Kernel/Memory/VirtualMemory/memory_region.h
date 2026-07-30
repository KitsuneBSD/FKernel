#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>

class Node;  // forward declaration (Node is in global scope, see node.h)

namespace fkernel {

struct MemoryRegion {
    uintptr_t start;
    uintptr_t end;
    PageFlags flags;
    const char* name;

    // File-backed MAP_SHARED fields — null for anonymous / private mappings
    Node*    backing_node{nullptr};
    uint64_t backing_offset{0};
    bool     is_shared{false};

    bool contains(uintptr_t addr) const {
        return addr >= start && addr < end;
    }
};

} // namespace fkernel
