#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

namespace MemoryManagement {

void PhysicalMemoryManager::add_region(PhysicalMemoryRegion* region) noexcept
{
    FK::enforcef(region != nullptr, "PMM: add_region received nullptr");
    FK::enforcef(region->page_count > 0, "PMM: add_region received region with 0 pages");
    FK::enforcef(!region->bitmap.is_valid(), "PMM: add_region received region with existing bitmap");

    if (region->page_count > max_region_in_pages) {
        LibC::uint64_t remaining_pages = region->page_count;
        LibC::uintptr_t current_base = region->base_addr;

        deallocate_region(region);

        while (remaining_pages > 0) {
            LibC::uint64_t split_count = (remaining_pages > max_region_in_pages) ? max_region_in_pages : remaining_pages;
            PhysicalMemoryRegion* subregion = allocate_region(current_base, split_count);
            FK::enforcef(subregion != nullptr, "PMM: Failed to allocate subregion");
            add_region(subregion);
            current_base += split_count * TOTAL_MEMORY_PAGE_SIZE;
            remaining_pages -= split_count;
        }
        return;
    }

    regions_.append(region);
}

void PhysicalMemoryManager::remove_region(LibC::uintptr_t phys_addr) noexcept
{
    FK::enforcef(phys_addr != 0, "PMM: remove_region received null physical address");

    auto* region = find_region(phys_addr);
    FK::enforcef(region != nullptr, "PMM: remove_region failed to find region for address %p", phys_addr);

    regions_.remove(region);
    deallocate_region(region);
}

PhysicalMemoryRegion* PhysicalMemoryManager::find_region(LibC::uintptr_t phys_addr) noexcept
{
    FK::enforcef(phys_addr != 0, "PMM: find_region received null physical address");

    for (auto& region : regions_) {
        if (phys_addr >= region.base_addr && phys_addr < region.base_addr + region.page_count * TOTAL_MEMORY_PAGE_SIZE) {
            return &region;
        }
    }
    return nullptr;
}

LibC::uintptr_t PhysicalMemoryManager::alloc_page() noexcept
{
    for (auto& region : regions_) {
        ensure_bitmap_allocated(region);

        LibC::uint64_t page_index = 0;
        if (!region.find_free_page(page_index))
            continue;

        LibC::uintptr_t phys_addr = region.base_addr + page_index * TOTAL_MEMORY_PAGE_SIZE;

        FK::alert_if_f(phys_addr == 0, "PMM: Skipping allocation of page at physical address 0");
        if (phys_addr == 0)
            continue;

        mark_pages(region, page_index, 1, true);
        return phys_addr;
    }

    FK::alert("PMM: Failed to allocate single page: out of memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    FK::alert_if_f(phys_addr == 0, "PMM: free_page received null physical address");
    if (phys_addr == 0)
        return;

    auto* region = find_region(phys_addr);
    FK::alert_if_f(region == nullptr, "PMM: free_page failed to find region for address %p", phys_addr);
    if (!region)
        return;

    FK::alert_if_f(!is_valid_aligned_address(phys_addr, region->base_addr, region->page_count),
        "PMM: free_page received misaligned or out-of-range address %p", phys_addr);
    if (!is_valid_aligned_address(phys_addr, region->base_addr, region->page_count))
        return;

    ensure_bitmap_allocated(*region);
    mark_pages(*region, (phys_addr - region->base_addr) / TOTAL_MEMORY_PAGE_SIZE, 1, false);
}

void PhysicalMemoryManager::ensure_bitmap_allocated(PhysicalMemoryRegion& region) noexcept
{
    FK::enforcef(region.is_allocated(), "PMR: Ensure bitmap called on unallocated region base=%p", region.base_addr);

    FK::enforcef(!region.bitmap.is_valid(), "PMM: Bitmap already valid for region base=%p", region.base_addr);

    FK::enforcef(region.bitmap_buffer == nullptr, "PMM: Ensure bitmap region already has bitmap_buffer set, potential leak");

    region.bitmap_word_count = (region.page_count + 63) / 64;

    void* mem = Falloc_zeroed(region.bitmap_word_count * sizeof(LibC::uint64_t), alignof(LibC::uint64_t));
    FK::enforcef(mem != nullptr, "PMM: Failed to allocate bitmap for region base=%p", region.base_addr);

    region.bitmap_buffer = reinterpret_cast<LibC::uint64_t*>(mem);

    region.bitmap.reset(region.bitmap_buffer, region.page_count);
    region.bitmap_allocated = true;
}

void PhysicalMemoryManager::mark_pages(PhysicalMemoryRegion& region, LibC::uint64_t page_index, LibC::uint64_t count, bool allocate) noexcept
{
    FK::enforcef(region.is_allocated(), "PMM: mark_pages called on unallocated region base=%p", region.base_addr);
    FK::enforcef(region.bitmap.is_valid(), "PMM: mark_pages called on region with invalid bitmap base=%p", region.base_addr);
    FK::enforcef(region.bitmap_allocated, "PMM: mark_pages called on region with unallocated bitmap base=%p", region.base_addr);
    FK::enforcef(page_index + count <= region.page_count, "PMM: mark_pages range out of bounds base=%p", region.base_addr);

    for (LibC::uint64_t i = 0; i < count; ++i) {
        if (allocate)
            region.mark_page(page_index + i);
        else
            region.unmark_page(page_index + i);
    }
}

LibC::size_t PhysicalMemoryManager::region_count() const noexcept
{
    LibC::size_t count = 0;
    for ([[maybe_unused]] auto& region : regions_) {
        ++count;
    }
    return count;
}

LibC::size_t PhysicalMemoryManager::allocated_region_count() const noexcept
{
    LibC::size_t count = 0;
    for (auto& region : regions_) {
        FK::enforcef(region.is_allocated(), "PMM: Allocated region count found region base=%p without allocation", region.base_addr);
        ++count;
    }
    return count;
}

LibC::uint64_t PhysicalMemoryManager::total_pages() const noexcept
{
    LibC::uint64_t total = 0;
    for (auto& region : regions_) {
        total += region.page_count;
    }
    return total;
}

LibC::uint64_t PhysicalMemoryManager::total_bytes() const noexcept
{
    return total_pages() * TOTAL_MEMORY_PAGE_SIZE;
}

LibC::uint64_t PhysicalMemoryManager::free_bytes() const noexcept
{
    return free_pages() * TOTAL_MEMORY_PAGE_SIZE;
}

LibC::uint64_t PhysicalMemoryManager::free_pages() const noexcept
{
    LibC::uint64_t free_count = 0;

    for (auto& region : regions_) {
        FK::enforcef(region.is_allocated(), "PMM: free_pages found unallocated region base=%p", region.base_addr);

        auto is_valid = bitmap_is_valid(region);

        free_count += is_valid ? (count_free_pages_in_bitmap_words(region, region_full_words(region)) + count_free_pages_in_remaining_bits(region, region_full_words(region), region_remaining_bits(region)))
                               : count_free_pages_full_region(region);
    }

    return free_count;
}

LibC::size_t PhysicalMemoryManager::count_used_pages() const noexcept
{
    LibC::size_t used_pages = 0;
    for (auto& region : regions_) {
        if (!region.bitmap.is_valid())
            continue;

        for (LibC::size_t i = 0; i < region.bitmap_word_count; ++i) {
            used_pages += __builtin_popcountll(region.bitmap_buffer[i]);
        }
    }
    return used_pages;
}

void PhysicalMemoryManager::log_memory_status() const noexcept
{
    LibC::uint64_t total_kb = total_bytes() / FK::KiB;
    LibC::uint64_t free_kb = free_bytes() / FK::KiB;

    LibC::size_t total_regions = region_count();
    LibC::size_t allocated_regions = allocated_region_count();

    LibC::size_t total_pages_count = total_pages();
    LibC::size_t used_pages_count = count_used_pages();
    LibC::size_t free_pages_count = total_pages_count - used_pages_count;

    Logf(LogLevel::INFO,
        "PMM: Memory status: total %llu KiB (%llu MiB), free %llu KiB (%llu MiB)",
        total_kb, total_kb / FK::KiB,
        free_kb, free_kb / FK::KiB);

    Logf(LogLevel::INFO, "PMM: Region status: regions total %lu, regions allocated %lu",
        total_regions, allocated_regions);

    Logf(LogLevel::INFO,
        "PMM: Pages used: %lu, free: %lu, total: %lu",
        used_pages_count, free_pages_count, total_pages_count);
}

}
