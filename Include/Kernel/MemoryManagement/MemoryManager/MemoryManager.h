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
    void allocate_and_map() noexcept;
    void populate_cache(multiboot2::TagMemoryMap const& mmap) noexcept;
    MMapCacheEntry* find_cache_entry(LibC::uintptr_t phys_addr) noexcept;

    MemoryManager() = default;

public:
    static MemoryManager& instance()
    {
        static MemoryManager instance;
        return instance;
    }

    void initialize(multiboot2::TagMemoryMap const& mmap);
    void* Kernel_Alloc(LibC::size_t size, LibC::size_t alignment = 16) noexcept;
    void Kernel_Free(void* ptr, LibC::size_t size) noexcept;
};
}

inline void* Kalloc(LibC::size_t size, LibC::size_t alignment) noexcept
{
    return MemoryManagement::MemoryManager::instance().Kernel_Alloc(size, alignment);
}
inline void Kfree(void* ptr, LibC::size_t size) noexcept
{
    MemoryManagement::MemoryManager::instance().Kernel_Free(ptr, size);
}
