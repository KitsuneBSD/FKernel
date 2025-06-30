#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

namespace MemoryManagement {
constexpr LibC::uint64_t total_page_size = 4096;
constexpr LibC::uint64_t max_region_bytes = 64 * FK::MiB;
constexpr LibC::uint64_t max_region_pages = max_region_bytes / total_page_size;

static PhysicalMemoryRegion* allocate_region(LibC::uintptr_t base_addr, LibC::uint64_t page_count) noexcept
{
    void* mem = Falloc(sizeof(PhysicalMemoryRegion), alignof(PhysicalMemoryRegion));
    if (!mem) {
        Log(LogLevel::ERROR, "PMM: allocate_region failed: out of memory");
        return nullptr;
    }

    auto* region = reinterpret_cast<PhysicalMemoryRegion*>(mem);
    region->base_addr = base_addr;
    region->page_count = page_count;
    region->bitmap = nullptr; // bitmap não alocado ainda (lazy)
    region->bitmap_size = 0;
    region->free_block = { 0, page_count };
    region->next = nullptr;
    region->prev = nullptr;

    return region;
}

static void deallocate_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region)
        return;

    if (region->bitmap) {
        Ffree(region->bitmap);
        region->bitmap = nullptr;
        region->bitmap_size = 0;
    }

    Ffree(region);
}

void PhysicalMemoryManager::add_region(PhysicalMemoryRegion* region) noexcept
{
    if (!region) {
        Log(LogLevel::ERROR, "PMM: add_region called with nullptr");
        return;
    }

    if (region->page_count > max_region_pages) {
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

    region->prev = region_list_tail;
    region->next = nullptr;

    if (region_list_tail)
        region_list_tail->next = region;
    else
        region_list_head = region;

    region_list_tail = region;
}

void PhysicalMemoryManager::initialize(multiboot2::TagMemoryMap const& mmap) noexcept
{
    constexpr LibC::uint64_t max_region_bytes = 64 * FK::MiB;
    constexpr LibC::uint64_t max_region_pages = max_region_bytes / total_page_size;

    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;
        if (!multiboot2::is_available(entry.type))
            continue;

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

            add_region(region);

            base += region_pages * total_page_size;
            total_pages -= region_pages;
        }
    }

    log_memory_status();
    Log(LogLevel::INFO, "PMM: Initialized with success");
}

void PhysicalMemoryManager::ensure_bitmap_allocated(PhysicalMemoryRegion* region) noexcept
{
    if (!region || region->bitmap)
        return;

    region->bitmap_size = (region->page_count + 63) / 64;

    void* mem = Falloc_zeroed(region->bitmap_size * sizeof(LibC::uint64_t), alignof(LibC::uint64_t));
    if (!mem) {
        Logf(LogLevel::ERROR, "PMM: Failed to allocate bitmap for region base=0x%lx", region->base_addr);
        region->bitmap_size = 0;
        return;
    }

    region->bitmap = reinterpret_cast<LibC::uint64_t*>(mem);
    Logf(LogLevel::TRACE, "PMM: Bitmap allocated for region base=0x%lx with %lu pages", region->base_addr, region->page_count);
}

bool PhysicalMemoryManager::find_free_pages_in_region(PhysicalMemoryRegion* region, LibC::uint64_t count, LibC::uint64_t& start_page) noexcept
{
    if (!region || !region->bitmap || count == 0 || count > region->page_count)
        return false;

    LibC::uint64_t run_length = 0;
    LibC::uint64_t run_start = 0;

    for (LibC::uint64_t page_idx = 0; page_idx < region->page_count; ++page_idx) {
        LibC::size_t word_idx = page_idx / 64;
        LibC::size_t bit_pos = page_idx % 64;
        bool is_free = ((region->bitmap[word_idx] & (1ULL << bit_pos)) == 0);

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

void PhysicalMemoryManager::mark_pages(PhysicalMemoryRegion* region, LibC::uint64_t start_page, LibC::uint64_t count, bool allocate) noexcept
{
    if (!region || !region->bitmap)
        return;

    if (start_page + count > region->page_count) {
        Logf(LogLevel::WARN, "PMM: mark_pages out of range: base=0x%lx, start_page=%lu, count=%lu",
            region->base_addr, start_page, count);
        return;
    }

    for (LibC::uint64_t i = 0; i < count; ++i) {
        LibC::size_t word_idx = (start_page + i) / 64;
        LibC::size_t bit_pos = (start_page + i) % 64;
        if (allocate)
            region->bitmap[word_idx] |= (1ULL << bit_pos);
        else
            region->bitmap[word_idx] &= ~(1ULL << bit_pos);
    }
}

LibC::uint64_t PhysicalMemoryManager::total_pages() const noexcept
{
    LibC::uint64_t total = 0;
    for (PhysicalMemoryRegion* cur = region_list_head; cur != nullptr; cur = cur->next)
        total += cur->page_count;
    return total;
}

LibC::uint64_t PhysicalMemoryManager::free_pages() const noexcept
{
    LibC::uint64_t free_count = 0;
    for (PhysicalMemoryRegion* region = region_list_head; region != nullptr; region = region->next) {
        if (!region->bitmap)
            continue;

        LibC::size_t full_words = region->page_count / 64;
        LibC::size_t remaining_bits = region->page_count % 64;

        for (LibC::size_t i = 0; i < full_words; ++i) {
            LibC::uint64_t word = region->bitmap[i];
            free_count += 64 - __builtin_popcountll(word);
        }
        if (remaining_bits > 0) {
            LibC::uint64_t last_word = region->bitmap[full_words];
            LibC::uint64_t mask = (1ULL << remaining_bits) - 1;
            last_word &= mask;
            free_count += remaining_bits - __builtin_popcountll(last_word);
        }
    }
    return free_count;
}

LibC::uint64_t PhysicalMemoryManager::total_bytes() const noexcept
{
    return total_pages() * total_page_size;
}

LibC::uint64_t PhysicalMemoryManager::free_bytes() const noexcept
{
    return free_pages() * total_page_size;
}

void PhysicalMemoryManager::log_memory_status() const noexcept
{
    LibC::uint64_t total_kb = total_bytes() / FK::KiB;
    LibC::uint64_t free_kb = free_bytes() / FK::KiB;

    LibC::size_t total_regions = region_count();
    LibC::size_t allocated_regions = allocated_region_count();

    Logf(LogLevel::INFO,
        "PMM: Memory status: total %llu KiB (%llu MiB), free %llu KiB (%llu MiB), regions total %lu, regions allocated %lu",
        total_kb, total_kb / FK::KiB,
        free_kb, free_kb / FK::KiB,
        total_regions, allocated_regions);
}

LibC::size_t PhysicalMemoryManager::region_count() const noexcept
{
    LibC::size_t count = 0;
    for (PhysicalMemoryRegion* cur = region_list_head; cur != nullptr; cur = cur->next) {
        count++;
    }
    return count;
}

LibC::size_t PhysicalMemoryManager::allocated_region_count() const noexcept
{
    LibC::size_t count = 0;
    for (PhysicalMemoryRegion* cur = region_list_head; cur != nullptr; cur = cur->next) {
        if (cur->bitmap) // se bitmap está alocado, a região está "alocada"
            count++;
    }
    return count;
}

LibC::uintptr_t PhysicalMemoryManager::alloc_page() noexcept
{
    for (PhysicalMemoryRegion* region = region_list_head; region != nullptr; region = region->next) {
        ensure_bitmap_allocated(region); // Garante bitmap alocado

        LibC::uint64_t start_page = 0;
        if (!find_free_pages_in_region(region, 1, start_page))
            continue;

        mark_pages(region, start_page, 1, true);
        LibC::uintptr_t phys_addr = region->base_addr + start_page * total_page_size;
        Logf(LogLevel::TRACE, "PMM: alloc_page allocated 0x%lx in region base=0x%lx", phys_addr, region->base_addr);
        return phys_addr;
    }

    Log(LogLevel::WARN, "PMM: alloc_page out of memory");
    return 0;
}

void PhysicalMemoryManager::free_page(LibC::uintptr_t phys_addr) noexcept
{
    for (PhysicalMemoryRegion* region = region_list_head; region != nullptr; region = region->next) {
        if (phys_addr < region->base_addr || phys_addr >= region->base_addr + region->page_count * total_page_size)
            continue;

        LibC::uintptr_t offset = phys_addr - region->base_addr;
        if (offset % total_page_size != 0) {
            Logf(LogLevel::WARN, "PMM: free_page unaligned address 0x%lx", phys_addr);
            return;
        }

        LibC::size_t page_idx = offset / total_page_size;
        ensure_bitmap_allocated(region); // Garante bitmap alocado antes de mexer

        mark_pages(region, page_idx, 1, false);

        Logf(LogLevel::TRACE, "PMM: free_page freed 0x%lx in region base=0x%lx", phys_addr, region->base_addr);
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

    for (PhysicalMemoryRegion* region = region_list_head; region != nullptr; region = region->next) {
        ensure_bitmap_allocated(region);

        LibC::uint64_t start_page = 0;
        if (!find_free_pages_in_region(region, count, start_page))
            continue;

        mark_pages(region, start_page, count, true);
        LibC::uintptr_t phys_addr = region->base_addr + start_page * total_page_size;
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

    for (PhysicalMemoryRegion* region = region_list_head; region != nullptr; region = region->next) {
        if (phys_addr < region->base_addr || phys_addr >= region->base_addr + region->page_count * total_page_size)
            continue;

        LibC::uintptr_t offset = phys_addr - region->base_addr;
        if (offset % total_page_size != 0) {
            Logf(LogLevel::WARN, "PMM: free_contiguous_pages unaligned address 0x%lx", phys_addr);
            return;
        }

        LibC::size_t page_idx = offset / total_page_size;
        if (page_idx + count > region->page_count) {
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

}
