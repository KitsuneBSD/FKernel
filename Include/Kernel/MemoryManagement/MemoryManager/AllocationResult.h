#pragma once

#include <LibC/stdint.h>

namespace MemoryManagement {

struct AllocationResult {
    LibC::uintptr_t virtual_address;
    LibC::uintptr_t physical_address;
};

}
