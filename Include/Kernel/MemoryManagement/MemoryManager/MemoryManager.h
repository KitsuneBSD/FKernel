#pragma once

#include "Kernel/Boot/multiboot2.h"
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/pair.h>

namespace MemoryManagement {

struct MMapCacheEntry {
    FK::Pair<LibC::uintptr_t, multiboot2::TagMemoryMap::Entry> data;
    FK::IntrusiveNode<MMapCacheEntry> list_node;
};

class MemoryManager {
private:
    bool is_initialized = false;
    FK::IntrusiveList<MMapCacheEntry, &MMapCacheEntry::list_node> mmap_cache;
    MMapCacheEntry* find_cache_entry(LibC::uintptr_t phys_addr) noexcept;
    MemoryManager() = default;

public:
    static MemoryManager& instance()
    {
        static MemoryManager instance;
        return instance;
    }

    void initialize(multiboot2::TagMemoryMap const& mmap);
    void* Kernel_Alloc(LibC::size_t size, LibC::uint64_t flags = PAGE_PRESENT | PAGE_RW);
    void Kernel_Free(void* addr, LibC::size_t size);
};
}
