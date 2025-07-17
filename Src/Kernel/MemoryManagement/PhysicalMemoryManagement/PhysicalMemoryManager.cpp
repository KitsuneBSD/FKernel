#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif

namespace MemoryManagement {

static inline bool is_valid_aligned_address(LibC::uintptr_t addr, LibC::uintptr_t base, LibC::uint64_t page_count) noexcept
{
    if (addr < base)
        return false;

    LibC::uintptr_t offset = addr - base;
    if (offset % TOTAL_MEMORY_PAGE_SIZE != 0)
        return false;
    return (offset / TOTAL_MEMORY_PAGE_SIZE) < page_count;
}

static PhysicalMemoryRegion* allocate_region(LibC::uintptr_t base_addr, LibC::uint64_t page_count) noexcept
{
    FK::enforcef(base_addr != 0, "PMM: allocate_region received base_addr=0");
    FK::enforcef(page_count > 0, "PMM: allocate_region received page_count=0");

    Logf(LogLevel::TRACE, "PMM: Allocate region base=%p, pages=%lu", base_addr, page_count);

    void* mem = Falloc_zeroed(sizeof(PhysicalMemoryRegion), alignof(PhysicalMemoryRegion));
    FK::enforcef(mem != nullptr, "PMM: Failed to allocate memory for region");

    auto* region = reinterpret_cast<PhysicalMemoryRegion*>(mem);
    FK::enforcef(!region->is_allocated(), "PMM: Region memory unexpectedly marked as allocated at %p", region);

    region->init(base_addr, page_count);
    Logf(LogLevel::TRACE, "PMM: Region initialized at %p with base=%p", region, base_addr);
    return region;
}

static void deallocate_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region) {
        FK::alert("PMM: deallocate_region received nullptr");
        return;
    }

    FK::enforcef(!region->is_allocated(), "PMM: Trying to deallocate region that is still allocated base=%p", region->base_addr);

    region->destroy();
    region->~PhysicalMemoryRegion();
    Ffree(region);
}

void PhysicalMemoryManager::add_region(PhysicalMemoryRegion* region) noexcept
{
    FK::enforcef(region != nullptr, "PMM: add_region received nullptr");
    FK::enforcef(region->page_count > 0, "PMM: add_region received region with 0 pages");
    FK::enforcef(!region->bitmap.is_valid(), "PMM: add_region received region with existing bitmap");

    Logf(LogLevel::TRACE, "PMM: Add Region base=%p pages=%lu", region->base_addr, region->page_count);

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

        if (phys_addr == 0) {
            FK::alert("PMM: Skipping allocation of page at physical address 0");
            continue;
        }

        mark_pages(region, page_index, 1, true);
        Logf(LogLevel::TRACE, "PMM: Allocated page %p", phys_addr);
        return phys_addr;
    }

    FK::alert("PMM: Failed to allocate single page: out of memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    if (phys_addr == 0) {
        FK::alert("PMM: free_page received null physical address");
        return;
    }

    auto* region = find_region(phys_addr);
    if (!region) {
        FK::alert_if_f(true, "PMM: free_page failed to find region for address %p", phys_addr);
        return;
    }

    if (!is_valid_aligned_address(phys_addr, region->base_addr, region->page_count)) {
        FK::alert_if_f(true, "PMM: free_page received misaligned or out-of-range address %p", phys_addr);
        return;
    }

    ensure_bitmap_allocated(*region);
    mark_pages(*region, (phys_addr - region->base_addr) / TOTAL_MEMORY_PAGE_SIZE, 1, false);
    Logf(LogLevel::TRACE, "PMM: Freed page %p", phys_addr);
}

void PhysicalMemoryManager::initialize(multiboot2::TagMemoryMap const& mmap) noexcept
{
    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        if (entry.length < TOTAL_MEMORY_PAGE_SIZE || entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = entry.base_addr;
        LibC::uint64_t total_pages = entry.length / TOTAL_MEMORY_PAGE_SIZE;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = allocate_region(base, region_pages);
            FK::enforcef(region != nullptr, "PMM: Failed to allocate PhysicalMemoryRegion for base=%p, pages=%lu", base, region_pages);

            add_region(region);

            if (!multiboot2::is_available(entry.type)) {
                if (region->allocated && !region->bitmap.is_valid()) {
                    ensure_bitmap_allocated(*region);
                    mark_pages(*region, 0, region_pages, true);
                }
            }

            base += region_pages * TOTAL_MEMORY_PAGE_SIZE;
            total_pages -= region_pages;
        }
    }

    log_memory_status();
    Log(LogLevel::INFO, "PMM: Initialized with success");
}

void PhysicalMemoryManager::ensure_bitmap_allocated(PhysicalMemoryRegion& region) noexcept
{
    FK::enforcef(region.is_allocated(), "PMR: Ensure bitmap called on unallocated region base=%p", region.base_addr);

    FK::enforcef(!region.bitmap.is_valid(), "PMM: Bitmap already valid for region base=%p", region.base_addr);

    FK::enforcef(region.bitmap_buffer == nullptr, "PMM: Ensure bitmap region already has bitmap_buffer set, potential leak");

    region.bitmap_word_count = (region.page_count + 63) / 64;
    Logf(LogLevel::TRACE, "PMM: Allocate bitmap: base=%p, words=%lu", region.base_addr, region.bitmap_word_count);

    void* mem = Falloc_zeroed(region.bitmap_word_count * sizeof(LibC::uint64_t), alignof(LibC::uint64_t));
    FK::enforcef(mem != nullptr, "PMM: Failed to allocate bitmap for region base=%p", region.base_addr);

    region.bitmap_buffer = reinterpret_cast<LibC::uint64_t*>(mem);

    region.bitmap.reset(region.bitmap_buffer, region.page_count);
    region.bitmap_allocated = true;
}

void PhysicalMemoryManager::mark_pages(PhysicalMemoryRegion& region, LibC::uint64_t page_index, LibC::uint64_t count, bool allocate) noexcept
{
    for (LibC::uint64_t i = 0; i < count; ++i) {
        if (allocate)
            region.mark_page(page_index + i);
        else
            region.unmark_page(page_index + i);
    }
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

LibC::size_t PhysicalMemoryManager::allocated_region_count() const noexcept
{
    LibC::size_t count = 0;
    for (auto& region : regions_) {
        FK::enforcef(region.is_allocated(), "PMM: Allocated region count found region base=%p without bitmap", region.base_addr);
        ++count;
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

LibC::size_t PhysicalMemoryManager::region_count() const noexcept
{
    LibC::size_t count = 0;
    for ([[maybe_unused]] auto& region : regions_) {
        ++count;
    }
    return count;
}

LibC::uintptr_t PhysicalMemoryManager::alloc_contiguous_pages(LibC::uint64_t count) noexcept
{
    FK::enforcef(count > 0, "PMM: alloc_contiguous_pages called with count=0");

    for (auto& region : regions_) {
        if (region.page_count < count)
            continue;

        ensure_bitmap_allocated(region);

        LibC::uint64_t max_start = region.page_count - count;
        LibC::uint64_t candidate = 0;

        while (candidate <= max_start) {
            bool block_free = true;

            for (LibC::uint64_t offset = 0; offset < count; ++offset) {
                if (region.is_page_used(candidate + offset)) {
                    candidate += offset + 1; // salto otimizado para próxima posição válida
                    block_free = false;
                    break;
                }
            }

            if (block_free) {
                LibC::uintptr_t phys_addr = region.base_addr + candidate * TOTAL_MEMORY_PAGE_SIZE;
                FK::alert_if_f(phys_addr == 0, "PMM: alloc_contiguous_pages skipping zero physical address");

                if (phys_addr == 0) {
                    ++candidate;
                    continue;
                }

                mark_pages(region, candidate, count, true);
                Logf(LogLevel::TRACE, "PMM: alloc_contiguous_pages allocated %lu pages at %p", count, phys_addr);
                return phys_addr;
            }
        }
    }

    Logf(LogLevel::WARN, "PMM: alloc_contiguous_pages out of memory for %lu pages", count);
    return 0;
}

void PhysicalMemoryManager::free_contiguous_pages(LibC::uintptr_t phys_addr, LibC::uint64_t count) noexcept
{
    FK::alert_if_f(count == 0, "PMM: free_contiguous_pages called with count=0");
    if (count == 0)
        return;

    auto* region = find_region(phys_addr);
    FK::alert_if_f(region == nullptr, "PMM: free_contiguous_pages address %p not found in any region", phys_addr);
    if (!region)
        return;

    LibC::uintptr_t offset = phys_addr - region->base_addr;
    FK::enforcef(offset % TOTAL_MEMORY_PAGE_SIZE == 0, "PMM: free_contiguous_pages unaligned address %p", phys_addr);

    FK::enforcef(offset / TOTAL_MEMORY_PAGE_SIZE + count <= region->page_count,
        "PMM: free_contiguous_pages range exceeds region for %p count %lu", phys_addr, count);

    ensure_bitmap_allocated(*region);

    mark_pages(*region, offset / TOTAL_MEMORY_PAGE_SIZE, count, false);

    Logf(LogLevel::TRACE, "PMM: free_contiguous_pages freed %lu pages at %p", count, phys_addr);
}

}
