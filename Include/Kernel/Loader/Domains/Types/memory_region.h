#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>

namespace fkernel::elf_domains {
struct MemoryRegion {
    uintptr_t start_vaddr;
    uintptr_t end_vaddr;
    PageFlags permissions;
    size_t file_offset;
    size_t file_size;
    size_t memory_size;
};
} // namespace fkernel::elf_domains