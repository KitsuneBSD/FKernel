#include "Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h"
#include "LibFK/types.h"
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

namespace MemoryManagement {
constexpr LibC::uint64_t total_page_size = 4096;
constexpr LibC::uint64_t max_region_bytes = 64 * static_cast<LibC::uint64_t>(FK::MiB);
constexpr LibC::uint64_t max_region_pages = max_region_bytes / total_page_size;

static PhysicalMemoryRegion* allocate_region(LibC::uintptr_t base_addr, LibC::uint64_t page_count) noexcept
{
    Logf(LogLevel::TRACE, "PMM: Allocating region base=0x%lx, pages=%lu", base_addr, page_count);

    void* mem = Falloc(sizeof(PhysicalMemoryRegion), alignof(PhysicalMemoryRegion));
    if (!mem) {
        Log(LogLevel::ERROR, "PMM: allocate_region failed: out of memory");
        return nullptr;
    }

    auto* region = reinterpret_cast<PhysicalMemoryRegion*>(mem);
    region->init(base_addr, page_count);

    return region;
}

static void deallocate_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region)
        return;

    Logf(LogLevel::TRACE, "PMM: Deallocating region base=0x%lx", region->base_addr);

    if (region->bitmap.is_valid()) {
        Logf(LogLevel::TRACE, "PMM: Freeing bitmap for region base=0x%lx", region->base_addr);
        Ffree(region->bitmap_buffer);
        region->destroy();
    }

    region->~PhysicalMemoryRegion();
    Ffree(region);
}

void PhysicalMemoryManager::add_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region) {
        Log(LogLevel::ERROR, "PMM: add_region called with nullptr");
        return;
    }

    Logf(LogLevel::TRACE, "PMM: add_region base=0x%lx pages=%lu", region->base_addr, region->page_count);

    if (region->page_count > max_region_pages) {
        Logf(LogLevel::TRACE, "PMM: Splitting region base=0x%lx pages=%lu", region->base_addr, region->page_count);
        LibC::uint64_t remaining_pages = region->page_count;
        LibC::uintptr_t current_base = region->base_addr;

        deallocate_region(region);

        while (remaining_pages > 0) {
            LibC::uint64_t split_count = (remaining_pages > max_region_pages) ? max_region_pages : remaining_pages;
            PhysicalMemoryRegion* subregion = allocate_region(current_base, split_count);
            if (!subregion) {
                Log(LogLevel::ERROR, "PMM: Failed to allocate subregion in add_region");
                return;
            }

            add_region(subregion);

            current_base += split_count * total_page_size;
            remaining_pages -= split_count;
        }
        return;
    }

    regions_.append(region);
}

void PhysicalMemoryManager::initialize(multiboot2::TagMemoryMap const& mmap) noexcept
{
    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        if (entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = entry.base_addr;
        LibC::uint64_t total_pages = entry.length / total_page_size;
        if (total_pages == 0)
            continue;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_pages) ? max_region_pages : total_pages;

            PhysicalMemoryRegion* region = allocate_region(base, region_pages);
            if (!region) {
                Log(LogLevel::ERROR, "PMM: Failed to allocate PhysicalMemoryRegion");
                break;
            }

            ensure_bitmap_allocated(*region);
            add_region(region);

            if (!multiboot2::is_available(entry.type)) {
                mark_pages(*region, 0, region_pages, true);
            }

            base += region_pages * total_page_size;
            total_pages -= region_pages;
        }
    }

    log_memory_status();
    Log(LogLevel::INFO, "PMM: Initialized with success");
}

void PhysicalMemoryManager::ensure_bitmap_allocated(PhysicalMemoryRegion& region) noexcept
{
    if (region.bitmap.is_valid())
        return;

    region.bitmap_word_count = (region.page_count + 63) / 64;
    Logf(LogLevel::TRACE, "PMM: Allocating bitmap: base=0x%lx, words=%lu", region.base_addr, region.bitmap_word_count);

    void* mem = Falloc_zeroed(region.bitmap_word_count * sizeof(LibC::uint64_t), alignof(LibC::uint64_t));
    if (!mem) {
        Logf(LogLevel::ERROR, "PMM: Failed to allocate bitmap for region base=0x%lx", region.base_addr);
        region.bitmap_word_count = 0;
        return;
    }

    region.bitmap_buffer = reinterpret_cast<LibC::uint64_t*>(mem);

    region.bitmap.reset(region.bitmap_buffer, region.page_count);
}

void PhysicalMemoryManager::mark_pages(PhysicalMemoryRegion& region, LibC::uint64_t start_page, LibC::uint64_t count, bool allocate) noexcept
{
    if (!region.bitmap.is_valid())
        return;

    if (start_page + count > region.page_count) {
        Logf(LogLevel::WARN, "PMM: mark_pages out of range: base=0x%lx, start_page=%lu, count=%lu",
            region.base_addr, start_page, count);
        return;
    }

    for (LibC::uint64_t i = 0; i < count; ++i) {
        if (allocate)
            region.bitmap.set(start_page + i);
        else
            region.bitmap.clear(start_page + i);
    }
}

bool PhysicalMemoryManager::find_free_pages_in_region(PhysicalMemoryRegion& region, LibC::uint64_t count, LibC::uint64_t& start_page) noexcept
{
    if (!region.bitmap.is_valid() || count == 0 || count > region.page_count)
        return false;

    LibC::uint64_t run_length = 0;
    LibC::uint64_t run_start = 0;

    for (LibC::uint64_t page_idx = 0; page_idx < region.page_count; ++page_idx) {
        bool is_free = !region.bitmap.test(page_idx);

        if (!is_free) {
            run_length = 0;
            continue;
        }

        if (run_length == 0)
            run_start = page_idx;

        if (++run_length == count) {
            start_page = run_start;
            return true;
        }
    }
    return false;
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
    return total_pages() * total_page_size;
}

LibC::uint64_t PhysicalMemoryManager::free_bytes() const noexcept
{
    return free_pages() * total_page_size;
}

LibC::size_t PhysicalMemoryManager::allocated_region_count() const noexcept
{
    LibC::size_t count = 0;
    for (auto& region : regions_) {
        if (region.is_allocated()) {
            ++count;
        } else {
            Logf(LogLevel::WARN, "PMM: allocated_region_count found region base=0x%lx without bitmap", region.base_addr);
        }
    }
    return count;
}

LibC::uint64_t PhysicalMemoryManager::free_pages() const noexcept
{
    LibC::uint64_t free_count = 0;
    for (auto& region : regions_) {
        if (!region.bitmap.is_valid()) {
            free_count += region.page_count;
            continue;
        }

        LibC::size_t full_words = region.page_count / 64;
        LibC::size_t remaining_bits = region.page_count % 64;

        for (LibC::size_t i = 0; i < full_words; ++i) {
            LibC::uint64_t word = region.bitmap_buffer[i];
            free_count += 64 - __builtin_popcountll(word);
        }
        if (remaining_bits > 0) {
            LibC::uint64_t last_word = region.bitmap_buffer[full_words];
            LibC::uint64_t mask = (1ULL << remaining_bits) - 1;
            last_word &= mask;
            free_count += remaining_bits - __builtin_popcountll(last_word);
        }
    }
    return free_count;
}

LibC::size_t PhysicalMemoryManager::count_used_pages() const noexcept
{
    LibC::size_t used_pages = 0;
    for (auto& region : regions_) {
        if (!region.bitmap.is_valid())
            continue; // sem controle de bitmap, não podemos contar páginas usadas

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

LibC::size_t PhysicalMemoryManager::region_count() const noexcept
{
    LibC::size_t count = 0;
    for ([[maybe_unused]] auto& region : regions_) {
        ++count;
    }
    return count;
}

LibC::uintptr_t PhysicalMemoryManager::alloc_page() noexcept
{
    for (auto& region : regions_) {
        ensure_bitmap_allocated(region);

        LibC::uint64_t start_page = 0;
        if (!find_free_pages_in_region(region, 1, start_page))
            continue;

        LibC::uintptr_t phys_addr = region.base_addr + start_page * total_page_size;
        if (phys_addr == 0) {
            Log(LogLevel::WARN, "PMM: alloc_page skipping zero physical address");
            continue;
        }

        mark_pages(region, start_page, 1, true);

        Logf(LogLevel::TRACE, "PMM: alloc_page allocated 0x%lx in region base=0x%lx", phys_addr, region.base_addr);
        return phys_addr;
    }

    Log(LogLevel::WARN, "PMM: alloc_page out of memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    for (auto& region : regions_) {
        if (phys_addr < region.base_addr || phys_addr >= region.base_addr + region.page_count * total_page_size)
            continue;

        LibC::uintptr_t offset = phys_addr - region.base_addr;
        if (offset % total_page_size != 0) {
            Logf(LogLevel::WARN, "PMM: free_page unaligned address 0x%lx", phys_addr);
            return;
        }

        LibC::size_t page_idx = offset / total_page_size;
        ensure_bitmap_allocated(region);

        mark_pages(region, page_idx, 1, false);

        Logf(LogLevel::TRACE, "PMM: free_page freed 0x%lx in region base=0x%lx", phys_addr, region.base_addr);
        return;
    }

    Logf(LogLevel::WARN, "PMM: free_page address 0x%lx not found in any region", phys_addr);
}

LibC::uintptr_t PhysicalMemoryManager::alloc_contiguous_pages(LibC::uint64_t count) noexcept
{
    if (count == 0) {
        Log(LogLevel::WARN, "PMM: alloc_contiguous_pages called with count=0");
        return 0;
    }

    for (auto& region : regions_) {
        ensure_bitmap_allocated(region);

        LibC::uint64_t start_page = 0;
        if (!find_free_pages_in_region(region, count, start_page))
            continue;

        mark_pages(region, start_page, count, true);
        LibC::uintptr_t phys_addr = region.base_addr + start_page * total_page_size;
        Logf(LogLevel::TRACE, "PMM: alloc_contiguous_pages allocated %lu pages at 0x%lx", count, phys_addr);
        return phys_addr;
    }

    Logf(LogLevel::WARN, "PMM: alloc_contiguous_pages out of memory for %lu pages", count);
    return 0;
}

void PhysicalMemoryManager::free_contiguous_pages(LibC::uintptr_t phys_addr, LibC::uint64_t count) noexcept
{
    if (count == 0) {
        Log(LogLevel::WARN, "PMM: free_contiguous_pages called with count=0");
        return;
    }

    for (auto& region : regions_) {
        if (phys_addr < region.base_addr || phys_addr >= region.base_addr + region.page_count * total_page_size)
            continue;

        LibC::uintptr_t offset = phys_addr - region.base_addr;
        if (offset % total_page_size != 0) {
            Logf(LogLevel::WARN, "PMM: free_contiguous_pages unaligned address 0x%lx", phys_addr);
            return;
        }

        LibC::size_t page_idx = offset / total_page_size;
        if (page_idx + count > region.page_count) {
            Logf(LogLevel::WARN, "PMM: free_contiguous_pages range exceeds region for 0x%lx count %lu", phys_addr, count);
            return;
        }

        ensure_bitmap_allocated(region);

        mark_pages(region, page_idx, count, false);

        Logf(LogLevel::TRACE, "PMM: free_contiguous_pages freed %lu pages at 0x%lx", count, phys_addr);
        return;
    }

    Logf(LogLevel::WARN, "PMM: free_contiguous_pages address 0x%lx not found in any region", phys_addr);
}

PhysicalMemoryRegion* PhysicalMemoryManager::find_region(LibC::uintptr_t phys_addr) noexcept
{
    for (auto& region : regions_) {
        if (phys_addr >= region.base_addr && phys_addr < region.base_addr + region.page_count * total_page_size) {
            return &region;
        }
    }
    return nullptr;
}

void PhysicalMemoryManager::remove_region(LibC::uintptr_t phys_addr) noexcept
{
    auto* region = find_region(phys_addr);
    if (!region) {
        Logf(LogLevel::WARN, "PMM: remove_region: region not found for address 0x%lx", phys_addr);
        return;
    }

    regions_.remove(region);
    deallocate_region(region);
}

void PhysicalMemoryRegion::init(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
{
    base_addr = base;
    page_count = pages;
    free_block = { 0, pages };
    bitmap.reset(nullptr, 0);
    bitmap_word_count = 0;
    allocated = true;
}

void PhysicalMemoryRegion::destroy() noexcept
{
    if (bitmap.is_valid()) {
        Ffree(bitmap_buffer);
    }
    bitmap.reset(nullptr, 0);
    bitmap_word_count = 0;
    allocated = false;
}
}
