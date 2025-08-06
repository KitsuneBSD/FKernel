#pragma once

#include "LibFK/intrusiveList.h"
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibC/stdint.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif

namespace MemoryManagement {
class PhysicalMemoryManager {
private:
    using RegionList = FK::IntrusiveList<PhysicalMemoryRegion, &PhysicalMemoryRegion::list_node>;
    RegionList regions_;

public:
    void add_region(PhysicalMemoryRegion* region) noexcept;
    PhysicalMemoryRegion* find_region(LibC::uintptr_t phys_addr) noexcept;
    void remove_region(PhysicalMemoryRegion* region) noexcept;

    void mark_pages(PhysicalMemoryRegion& region, LibC::uint64_t start_page, LibC::uint64_t count, bool allocate) noexcept;
    PhysicalMemoryRegion* find_region_containing(LibC::uintptr_t addr) noexcept;
    LibC::size_t count_used_pages() const noexcept;
    LibC::size_t allocated_region_count() const noexcept;
    void ensure_bitmap_allocated(PhysicalMemoryRegion& region) noexcept;
    void remove_region(LibC::uintptr_t phys_addr) noexcept;

    RegionList regions()
    {
        return regions_;
    }

    static PhysicalMemoryManager& instance() noexcept
    {
        static PhysicalMemoryManager s_instance;
        return s_instance;
    }

    static inline bool is_valid_aligned_address(LibC::uintptr_t addr, LibC::uintptr_t base, LibC::uint64_t page_count) noexcept
    {
        if (FK::alert_if_f(base == 0, "PMM: is_valid_aligned_address base %zu cannot be zero"))
            return false;

        if (FK::alert_if_f(page_count == 0, "PMM: is_valid_aligned_address page_count must be positive"))
            return false;

        if (addr < base)
            return false;

        LibC::uintptr_t offset = addr - base;
        if (offset % TOTAL_MEMORY_PAGE_SIZE != 0)
            return false;

        return (offset / TOTAL_MEMORY_PAGE_SIZE) < page_count;
    }

    static bool bitmap_is_valid(PhysicalMemoryRegion const& region) noexcept
    {
        if (FK::alert_if_f(!region.is_allocated(), "PMM: bitmap_is_valid called on unallocated region base=%p", region.base_addr))
            return false;
        return region.bitmap.is_valid();
    }

    static LibC::uint64_t count_free_pages_full_region(PhysicalMemoryRegion const& region) noexcept
    {
        return region.page_count;
    }

    static LibC::size_t region_full_words(PhysicalMemoryRegion const& region) noexcept
    {
        return region.page_count / 64;
    }

    static LibC::size_t region_remaining_bits(PhysicalMemoryRegion const& region) noexcept
    {
        return region.page_count % 64;
    }

    static LibC::uint64_t count_free_pages_in_bitmap_words(PhysicalMemoryRegion const& region, LibC::size_t full_words) noexcept
    {
        LibC::uint64_t count = 0;
        for (LibC::size_t i = 0; i < full_words; ++i) {
            count += 64 - __builtin_popcountll(region.bitmap_buffer[i]);
        }
        return count;
    }

    static LibC::uint64_t count_free_pages_in_remaining_bits(PhysicalMemoryRegion const& region, LibC::size_t full_words, LibC::size_t remaining_bits) noexcept
    {
        if (remaining_bits == 0)
            return 0;

        LibC::uint64_t last_word = region.bitmap_buffer[full_words];
        LibC::uint64_t mask = (1ULL << remaining_bits) - 1;
        last_word &= mask;
        return remaining_bits - __builtin_popcountll(last_word);
    }

    static PhysicalMemoryRegion* allocate_region(LibC::uintptr_t base_addr, LibC::uint64_t page_count) noexcept
    {
        if (FK::alert_if_f(base_addr == 0, "PMM: allocate_region received base_addr=0"))
            return nullptr;

        if (FK::alert_if_f(page_count == 0, "PMM: allocate_region received page_count=0"))
            return nullptr;

        void* mem = Falloc_zeroed(sizeof(PhysicalMemoryRegion), alignof(PhysicalMemoryRegion));
        if (FK::alert_if_f(mem == nullptr, "PMM: Failed to allocate memory for region"))
            return nullptr;

        auto* region = reinterpret_cast<PhysicalMemoryRegion*>(mem);
        if (FK::alert_if_f(region->is_allocated(), "PMM: Region memory unexpectedly marked as allocated at %p", region))
            return nullptr;

        region->init(base_addr, page_count);
        return region;
    }

    static void deallocate_region(PhysicalMemoryRegion* region) noexcept
    {
        if (FK::alert_if_f(region == nullptr, "PMM: deallocate_region received nullptr"))
            return;

        if (FK::alert_if_f(!region->is_allocated(), "PMM: Trying to deallocate region that is not allocated base=%p", region->base_addr))
            return;

        region->destroy();
        region->~PhysicalMemoryRegion();
        Ffree(region);
    }

    LibC::uintptr_t alloc_page() noexcept;
    void free_page(LibC::uintptr_t phys_addr) noexcept;

    void log_memory_status() const noexcept;

    LibC::uint64_t total_pages() const noexcept;
    LibC::uint64_t free_pages() const noexcept;
    LibC::uint64_t total_bytes() const noexcept;
    LibC::uint64_t free_bytes() const noexcept;

    LibC::size_t region_count() const noexcept;
};
}
