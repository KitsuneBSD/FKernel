#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif

namespace MemoryManagement {
static PhysicalMemoryRegion* allocate_region(LibC::uintptr_t base_addr, LibC::uint64_t page_count) noexcept
{
    Logf(LogLevel::TRACE, "PMM: Allocate region base=0x%lx, pages=%lu", base_addr, page_count);

    void* mem = Falloc_zeroed(sizeof(PhysicalMemoryRegion), alignof(PhysicalMemoryRegion));
    if (!mem) {
        Log(LogLevel::ERROR, "PMM: Allocate region failed: out of memory");
        return nullptr;
    }

    auto* region = reinterpret_cast<PhysicalMemoryRegion*>(mem);

    if (region->is_allocated()) {
        Logf(LogLevel::ERROR, "PMM: Allocate region failed: already allocated region at %p", region);
        Ffree(region);
        return nullptr;
    }
    region->init(base_addr, page_count);

    Logf(LogLevel::TRACE, "PMM: Region allocated at %p", region);
    return region;
}

static void deallocate_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region)
        return;

    Logf(LogLevel::TRACE, "PMM: Deallocate region base=0x%lx", region->base_addr);

    if (!region->is_allocated()) {
        Logf(LogLevel::WARN, "PMM: Deallocate region called on unallocated region at %p", region);
        return;
    }

    region->destroy();

    region->~PhysicalMemoryRegion();
    Ffree(region);
}

void PhysicalMemoryManager::add_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region) {
        Log(LogLevel::ERROR, "PMM: Add Region called with nullptr");
        return;
    }

    if (region->bitmap.is_valid()) {
        Logf(LogLevel::WARN, "PMM: Region already had bitmap before add_region base=0x%lx", region->base_addr);
        return;
    }

    Logf(LogLevel::TRACE, "PMM: Add Region base=0x%lx pages=%lu", region->base_addr, region->page_count);

    if (region->page_count > max_region_in_pages) {
        Logf(LogLevel::TRACE, "PMM: Split region base=0x%lx pages=%lu", region->base_addr, region->page_count);
        LibC::uint64_t remaining_pages = region->page_count;
        LibC::uintptr_t current_base = region->base_addr;

        deallocate_region(region);

        while (remaining_pages > 0) {
            LibC::uint64_t split_count = (remaining_pages > max_region_in_pages) ? max_region_in_pages : remaining_pages;
            PhysicalMemoryRegion* subregion = allocate_region(current_base, split_count);
            if (!subregion) {
                Log(LogLevel::ERROR, "PMM: Failed to allocate subregion in add_region");
                return;
            }

            add_region(subregion);

            current_base += split_count * TOTAL_MEMORY_PAGE_SIZE;
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

        if (entry.length < TOTAL_MEMORY_PAGE_SIZE || entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = entry.base_addr;
        LibC::uint64_t total_pages = entry.length / TOTAL_MEMORY_PAGE_SIZE;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = allocate_region(base, region_pages);
            if (!region) {
                Logf(LogLevel::ERROR, "PMM: Failed to allocate PhysicalMemoryRegion for base=0x%lx, pages=%lu", base, region_pages);
                break;
            }

            add_region(region);

            if (!multiboot2::is_available(entry.type)) {
                ensure_bitmap_allocated(*region);
                mark_pages(*region, 0, region_pages, true);
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

    if (!region.is_allocated()) {
        Logf(LogLevel::WARN, "PMR: Ensure bitmap called on unallocated region base=0x%lx", region.base_addr);
        return;
    }

    if (region.bitmap.is_valid()) {
        Logf(LogLevel::TRACE, "PMM: Bitmap already valid for region base=0x%lx", region.base_addr);
        return;
    }

    if (region.bitmap_buffer) {
        Logf(LogLevel::ERROR, "PMM: Ensure bitmap region already has bitmap_buffer set, potential leak");
        return;
    }

    region.bitmap_word_count = (region.page_count + 63) / 64;
    Logf(LogLevel::TRACE, "PMM: Allocate bitmap: base=0x%lx, words=%lu", region.base_addr, region.bitmap_word_count);

    void* mem = Falloc_zeroed(region.bitmap_word_count * sizeof(LibC::uint64_t), alignof(LibC::uint64_t));
    if (!mem) {
        Logf(LogLevel::ERROR, "PMM: Failed to allocate bitmap for region base=0x%lx", region.base_addr);
        region.bitmap_word_count = 0;
        return;
    }

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
        if (!region.is_allocated()) {
            Logf(LogLevel::WARN, "PMM: Allocated region count found region base=0x%lx without bitmap", region.base_addr);
        }
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

        LibC::uint64_t page_index = 0;
        if (!region.find_free_page(page_index))
            continue;

        LibC::uintptr_t phys_addr = region.base_addr + page_index * TOTAL_MEMORY_PAGE_SIZE;
        if (phys_addr == 0) {
            Log(LogLevel::WARN, "PMM: Alloc page skipping zero physical address");
            continue;
        }

        mark_pages(region, page_index, 1, true);

        Logf(LogLevel::TRACE, "PMM: Alloc page allocated 0x%lx in region base=0x%lx", phys_addr, region.base_addr);
        return phys_addr;
    }

    Log(LogLevel::WARN, "PMM: Alloc page out of memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    for (auto& region : regions_) {
        if (phys_addr < region.base_addr || phys_addr >= region.base_addr + region.page_count * TOTAL_MEMORY_PAGE_SIZE)
            continue;

        LibC::uintptr_t offset = phys_addr - region.base_addr;
        if (offset % TOTAL_MEMORY_PAGE_SIZE != 0) {
            Logf(LogLevel::WARN, "PMM: Free page unaligned address 0x%lx", phys_addr);
            return;
        }

        LibC::size_t page_idx = offset / TOTAL_MEMORY_PAGE_SIZE;
        ensure_bitmap_allocated(region);

        mark_pages(region, page_idx, 1, false);

        Logf(LogLevel::TRACE, "PMM: Free page freed 0x%lx in region base=0x%lx", phys_addr, region.base_addr);
        return;
    }

    Logf(LogLevel::WARN, "PMM: Free page address 0x%lx not found in any region", phys_addr);
}

LibC::uintptr_t PhysicalMemoryManager::alloc_contiguous_pages(LibC::uint64_t count) noexcept
{
    if (count == 0) {
        Log(LogLevel::WARN, "PMM: alloc_contiguous_pages called with count=0");
        return 0;
    }

    for (auto& region : regions_) {
        ensure_bitmap_allocated(region);

        if (region.page_count < count)
            continue;

        LibC::uint64_t max_start = region.page_count - count;

        for (LibC::uint64_t candidate = 0; candidate <= max_start; ++candidate) {
            bool block_free = true;

            for (LibC::uint64_t offset = 0; offset < count; ++offset) {
                if (region.is_page_used(candidate + offset)) {
                    block_free = false;
                    candidate += offset; // pular para a próxima possível
                    break;
                }
            }

            if (block_free) {
                LibC::uintptr_t phys_addr = region.base_addr + candidate * TOTAL_MEMORY_PAGE_SIZE;
                if (phys_addr == 0) {
                    Log(LogLevel::WARN, "PMM: alloc_contiguous_pages skipping zero physical address");
                    continue;
                }

                mark_pages(region, candidate, count, true);
                Logf(LogLevel::TRACE, "PMM: alloc_contiguous_pages allocated %lu pages at 0x%lx", count, phys_addr);
                return phys_addr;
            }
        }
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

    bool found_region = false;

    for (auto& region : regions_) {
        if (phys_addr >= region.base_addr && phys_addr < region.base_addr + region.page_count * TOTAL_MEMORY_PAGE_SIZE) {
            found_region = true;

            LibC::uintptr_t offset = phys_addr - region.base_addr;
            if (offset % TOTAL_MEMORY_PAGE_SIZE != 0) {
                Logf(LogLevel::WARN, "PMM: free_contiguous_pages unaligned address 0x%lx", phys_addr);
                return;
            }

            LibC::size_t page_idx = offset / TOTAL_MEMORY_PAGE_SIZE;
            if (page_idx + count > region.page_count) {
                Logf(LogLevel::WARN, "PMM: free_contiguous_pages range exceeds region for 0x%lx count %lu", phys_addr, count);
                return;
            }

            ensure_bitmap_allocated(region);

            mark_pages(region, page_idx, count, false);

            Logf(LogLevel::TRACE, "PMM: free_contiguous_pages freed %lu pages at 0x%lx", count, phys_addr);
            return;
        }
    }

    if (!found_region) {
        Logf(LogLevel::WARN, "PMM: free_contiguous_pages address 0x%lx not found in any region", phys_addr);
    }
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

void PhysicalMemoryManager::remove_region(LibC::uintptr_t phys_addr) noexcept
{
    auto* region = find_region(phys_addr);
    if (!region) {
        Logf(LogLevel::WARN, "PMM: Remove region: region not found for address 0x%lx", phys_addr);
        return;
    }

    regions_.remove(region);
    deallocate_region(region);
}

PhysicalMemoryRegion* PhysicalMemoryManager::find_region_containing(LibC::uintptr_t addr) noexcept
{
    for (auto it = regions_.begin(); it != regions_.end(); ++it) {
        PhysicalMemoryRegion& region = *it;

        LibC::uintptr_t region_start = region.base_addr;
        LibC::uintptr_t region_end = region_start + region.page_count * TOTAL_MEMORY_PAGE_SIZE;

        if (addr >= region_start && addr < region_end) {
            return &region;
        }
    }
    return nullptr;
}

}
